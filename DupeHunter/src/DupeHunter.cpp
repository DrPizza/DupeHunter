// DupeHunter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

typedef std::map<unsigned __int64, std::vector<std::wstring> > size_map_type;

bool permitted_name(const std::wstring& name, const std::vector<boost::wregex>& include_patterns, const std::vector<boost::wregex>& exclude_patterns) {
	for(auto ipit(include_patterns.cbegin()), ipend(include_patterns.cend()); ipit != ipend; ++ipit) {
		if(boost::regex_match(name, *ipit)) {
			for(auto epit(exclude_patterns.cbegin()), epend(exclude_patterns.cend()); epit != epend; ++epit) {
				if(boost::regex_match(name, *epit)) {
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

unsigned __int64 populate_files(size_map_type& files, const std::vector<boost::wregex>& include_patterns, const std::vector<boost::wregex>& exclude_patterns, const std::wstring& basePath) {
	WIN32_FILE_ATTRIBUTE_DATA attributes = {0};
	::GetFileAttributesExW(basePath.c_str(), GetFileExInfoStandard, &attributes);

	if((attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY) {
		DWORD buffer_size(::GetFullPathNameW(basePath.c_str(), 0, nullptr, nullptr));
		std::unique_ptr<wchar_t[]> buffer(new wchar_t[buffer_size]);
		wchar_t* file_name(nullptr);
		::GetFullPathNameW(basePath.c_str(), buffer_size, buffer.get(), &file_name);
		if(permitted_name(file_name, include_patterns, exclude_patterns)) {
			files[((static_cast<unsigned __int64>(attributes.nFileSizeHigh) << 32) + static_cast<unsigned __int64>(attributes.nFileSizeLow))].push_back(basePath);
			return 1;
		}
		return 0;
	}

	unsigned __int64 count(0);
	const std::wstring search_path(basePath + (basePath[basePath.size() - 1] == L'\\' ? L"*" : L"\\*"));
	WIN32_FIND_DATAW found = {0};
	HANDLE finder(::FindFirstFileW(search_path.c_str(), &found));
	ON_BLOCK_EXIT([=] { ::FindClose(finder); });
	do {
		static const wchar_t* dot(L".");
		static const wchar_t* dotdot(L"..");
		if(0 == std::wcscmp(found.cFileName, dot) || 0 == std::wcscmp(found.cFileName, dotdot)) {
			continue;
		}
		const std::wstring file_path(basePath + (basePath[basePath.size() - 1] == L'\\' ? L"" : L"\\") + found.cFileName);
		if((found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
			if((found.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != FILE_ATTRIBUTE_REPARSE_POINT) {
				count += populate_files(files, include_patterns, exclude_patterns, file_path);
			}
		}
		else {
			if(permitted_name(found.cFileName, include_patterns, exclude_patterns)) {
				files[((static_cast<unsigned __int64>(found.nFileSizeHigh) << 32) + static_cast<unsigned __int64>(found.nFileSizeLow))].push_back(file_path);
				++count;
			}
		}
	}
	while(FALSE != ::FindNextFileW(finder, &found));
	return count;
}

bool read_multi_file(const std::vector<HANDLE>& files, const std::vector<unsigned __int8*>& buffers, const size_t buffer_size, std::vector<DWORD>& bytes_read) {
	bool result(true);
	for(size_t i(0); i < files.size(); ++i) {
		result &= FALSE != ::ReadFile(files[i], buffers[i], buffer_size, &bytes_read[i], NULL) && 0 != bytes_read[i];
	}
	return result;
}

template<typename T>
T round_to_next_multiple(T num, T factor) {
	return ((num + factor - 1) / factor) * factor;
}

template<typename T>
T round_to_previous_multiple(T num, T factor) {
	return (num / factor) * factor;
}

std::vector<std::vector<std::wstring> > n_way_compare(unsigned __int64 file_size, std::vector<std::wstring>& names, void* buffer, const size_t total_buffer_size) {
	// we size the buffer such that it can hold as much of each file as possible, subject to the constraint that it must not use more than roughly our total buffer size
	// if we can't read each file in totality, we just carve up our buffer space evenly
	if(names.size() > total_buffer_size) {
		throw std::exception("Buffer too small for this number of files");
	}
	static const unsigned __int64 sector_size(4096); // TODO get the right size
	const unsigned __int64 rounded_file_size(round_to_next_multiple(file_size, sector_size));
	const bool aligned_reads    = (static_cast<unsigned __int64>(names.size()) * sector_size      ) <= static_cast<unsigned __int64>(total_buffer_size);
	const bool read_whole_files = (static_cast<unsigned __int64>(names.size()) * rounded_file_size) <= static_cast<unsigned __int64>(total_buffer_size);
	const unsigned __int64 buffer_size = aligned_reads ? (read_whole_files ? rounded_file_size
	                                                                       : round_to_previous_multiple(total_buffer_size / (static_cast<unsigned __int64>(names.size()) * sector_size), sector_size))
	                                                   : static_cast<unsigned __int64>(total_buffer_size) / static_cast<unsigned __int64>(names.size());

	std::vector<HANDLE> files(names.size());
	std::set<unsigned __int64> file_ids;
	for(size_t i(0); i < names.size();) {
		files[i] = ::CreateFileW(names[i].c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | (aligned_reads ? FILE_FLAG_NO_BUFFERING : 0), 0);
		if(files[i] == INVALID_HANDLE_VALUE) {
			std::wcerr << L"Could not open file " << names[i] << L" with error 0x" << std::hex << ::GetLastError() << std::dec << L", ignoring" << std::endl;
			names.erase(names.begin() + i);
			files.erase(files.begin() + i);
			continue;
		}
		BY_HANDLE_FILE_INFORMATION info = {0};
		::GetFileInformationByHandle(files[i], &info);
		// skip hard linked "duplicates" as they occupy zero additional space
		// TODO it would be nice to keep their names hanging around, for reporting purposes.
		const unsigned __int64 file_id((static_cast<unsigned __int64>(info.nFileIndexHigh) << 32) + info.nFileIndexLow);
		if(file_ids.find(file_id) != file_ids.end()) {
			std::wcerr << L"Skipping file " << names[i] << L" due to hard links" << std::endl;
			::CloseHandle(files[i]);
			names.erase(names.begin() + i);
			files.erase(files.begin() + i);
			continue;
		}
		file_ids.insert(file_id);
		++i;
	}
	ON_BLOCK_EXIT([=] {
		std::for_each(files.begin(), files.end(), &::CloseHandle);
	});

	if(names.size() <= 1) {
		return std::vector<std::vector<std::wstring> >();
	}

	std::vector<unsigned __int8*> buffers(names.size());
	for(size_t i(0); i < names.size(); ++i) {
		buffers[i] = static_cast<unsigned __int8*>(buffer) + (i * buffer_size);
	}
	std::vector<DWORD> bytes_read(names.size());

	// even with vector<bool>'s compact representation, this can be a large array
	// (in the order of hundreds of MB), so while a rectangular representation
	// would be easier, I will make it triangular, and halve memory usage.
	// This means that comparison_result[a][b] stores the result of the 
	// comparison between files[a] and files[a + b + 1]
	typedef std::vector<std::vector<bool> > comparison_result_type;
	comparison_result_type comparison_results(names.size());
	for(size_t i(0), lim(comparison_results.size()); i < lim; ++i) {
		comparison_results[i].resize(names.size() - i - 1, true);
	}

	bool work_to_do(true);
	while(work_to_do && false != read_multi_file(files, buffers, buffer_size, bytes_read)) {
		work_to_do = false;
		for(size_t i(0), lim(names.size()); i < lim - 1; ++i) {
			for(size_t j(i + 1); j < lim; ++j) {
				if(comparison_results[i][j - i - 1] != false) {
					comparison_results[i][j - i - 1] = bytes_read[i] == bytes_read[j] ? 0 == std::memcmp(buffers[i], buffers[j], bytes_read[i])
					                                                                  : false;
				}
				work_to_do |= comparison_results[i][j - i - 1];
			}
		}
	}

	std::vector<std::vector<std::wstring> > duplicate_sets;
	std::vector<bool> already_used(names.size(), false);
	for(size_t i(0), lim(names.size()); i < lim - 1; ++i) {
		if(!already_used[i]) {
			std::vector<std::wstring> duplicates;
			duplicates.push_back(names[i]);
			for(size_t j(i + 1); j < lim; ++j) {
				if(comparison_results[i][j - i - 1]) {
					already_used[j] = true;
					duplicates.push_back(names[j]);
				}
			}
			if(duplicates.size() > 1) {
				duplicate_sets.push_back(duplicates);
			}
		}
	}
	return duplicate_sets;
}

int wmain(int argc, wchar_t* argv[])
try {
	namespace po = boost::program_options;

	size_t buffer_size(0);
	std::vector<std::wstring> directories;
	std::vector<std::wstring> inc_patterns;
	std::vector<std::wstring> inc_epatterns;
	std::vector<std::wstring> exc_patterns;
	std::vector<std::wstring> exc_epatterns;

	po::options_description desc("Allowed options");
	desc.add_options()
		("help",                                                                             "show this message")
		("buffer-size", po::wvalue<size_t>(&buffer_size)->default_value(1024 * 1024 * 1024), "set maximum buffer size")
		("source",      po::wvalue<std::vector<std::wstring> >(&directories)->composing(),   "directories to search")
		("include,i",   po::wvalue<std::vector<std::wstring> >(&inc_patterns)->composing(),  "wildcard filename pattern to include")
		("einclude,I",  po::wvalue<std::vector<std::wstring> >(&inc_epatterns)->composing(), "regex filename pattern to include")
		("exclude,x",   po::wvalue<std::vector<std::wstring> >(&exc_patterns)->composing(),  "wildcard filename pattern to exclude")
		("eexclude,X",  po::wvalue<std::vector<std::wstring> >(&exc_epatterns)->composing(), "regex filename pattern to exclude")
	;

	po::positional_options_description p;
	p.add("source", -1);

	po::variables_map vm;
	po::store(po::wcommand_line_parser(argc, argv).options(desc).style(po::command_line_style::unix_style).positional(p).run(), vm);
	po::notify(vm);

	if(vm.count("help")) {
		std::cout << desc << std::endl;
		return -1;
	}

	if(!vm.count("source")) {
		std::cerr << desc << std::endl;
		return -1;
	}

	void* buffer(::VirtualAlloc(nullptr, buffer_size,  MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	ON_BLOCK_EXIT([=] { ::VirtualFree(buffer, 0, MEM_RELEASE); });

	if(inc_patterns.size() == 0 && inc_epatterns.size() == 0) {
		inc_patterns.push_back(L"*");
	}

	std::vector<boost::wregex> include_patterns;
	std::vector<boost::wregex> exclude_patterns;

	const boost::wregex wildcard_transform(L"([+{}()\\[\\]$\\^|])|(\\*)|(\\?)|(\\.)|([\\\\/:])");
	// this has to be double-escaped because we want the result to be properly escaped to produce a regex itself.
	// It would be nice to have raw strings!
	const std::wstring wildcard_replacement(L"(?1\\\\$1)(?2[^\\\\\\\\/\\:]*)(?3[^\\\\\\\\/\\:])(?4\\(\\?\\:\\\\.|$\\))(?5[\\\\\\\\\\\\/\\:])");

	for(auto it(inc_patterns.cbegin()), end(inc_patterns.cend()); it != end; ++it) {
		include_patterns.push_back(boost::wregex(boost::regex_replace(*it, wildcard_transform, wildcard_replacement, boost::format_all), boost::regex::icase));
	}
	for(auto it(exc_patterns.cbegin()), end(exc_patterns.cend()); it != end; ++it) {
		exclude_patterns.push_back(boost::wregex(boost::regex_replace(*it, wildcard_transform, wildcard_replacement, boost::format_all), boost::regex::icase));
	}
	for(auto it(inc_epatterns.cbegin()), end(inc_epatterns.cend()); it != end; ++it) {
		include_patterns.push_back(boost::wregex(*it, boost::regex::icase));
	}
	for(auto it(exc_epatterns.cbegin()), end(exc_epatterns.cend()); it != end; ++it) {
		exclude_patterns.push_back(boost::wregex(*it, boost::regex::icase));
	}

	size_map_type files;
	unsigned __int64 total_files(0);

	for(auto it(directories.cbegin()), end(directories.cend()); it != end; ++it) {
		std::wcout << L"Searching " << *it << std::endl;
		total_files += populate_files(files, include_patterns, exclude_patterns, *it);
	}

	std::wcout << L"Found " << total_files << L" files matching search criteria" << std::endl;
	unsigned __int64 files_read(total_files);
	for(auto it(files.cbegin()), end(files.cend()); it != end;) {
		if(it->first == 0 || it->second.size() == 1) {
			files_read -= it->second.size();
			files.erase(it++);
		}
		else {
			++it;
		}
	}
	std::wcout << L"Comparing " << files_read << L" files with non-unique sizes" << std::endl;
	for(auto it(files.begin()), end(files.end()); it != end; ++it) {
		std::wcout << L"Comparing " << it->second.size() << L" files of size " << it->first << std::endl;
		std::vector<std::vector<std::wstring> > duplicates(n_way_compare(it->first, it->second, buffer, buffer_size));
		size_t count(0);
		for(auto it(duplicates.cbegin()), end(duplicates.cend()); it != end; ++it) {
			std::wcout << L"\tDuplicate set " << ++count << std::endl;
			for(std::vector<std::wstring>::const_iterator nit(it->begin()), nend(it->end()); nit != nend; ++nit) {
				std::wcout << L"\t\t" << *nit << std::endl;
			}
		}
	}

	return 0;
}
catch(std::exception& e) {
	std::cerr << "Caught exception" << std::endl;
	std::cerr << e.what() << std::endl;
	return -2;
}
