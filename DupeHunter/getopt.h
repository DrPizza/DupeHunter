#ifndef GETOPT_H
#define GETOPT_H

// Copyright (C) 2006 Peter Bright <drpizza@quiscalusmexicanus.org>
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

// Copyright (c) 2002 Todd C. Miller <Todd.Miller@courtesan.com>
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
// Sponsored in part by the Defense Advanced Research Projects
// Agency (DARPA) and Air Force Research Laboratory, Air Force
// Materiel Command, USAF, under agreement number F39502-99-1-0512.

// Copyright (c) 2000 The NetBSD Foundation, Inc.
// All rights reserved.
//
// This code is derived from software contributed to The NetBSD Foundation
// by Dieter Baron and Thomas Klausner.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. All advertising materials mentioning features or use of this software
//    must display the following acknowledgement:
//        This product includes software developed by the NetBSD
//        Foundation, Inc. and its contributors.
// 4. Neither the name of The NetBSD Foundation nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
// ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
// BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <string>

#include <utility/string.hpp>
#include <utility/widthConversion.hpp>

#define	INORDER 	(int)1

template<typename T>
struct empty_string
{
};

template<>
struct empty_string<char>
{
	static const char* const value;
};

const char* const empty_string<char>::value = "";

template<>
struct empty_string<wchar_t>
{
	static const wchar_t* const value;
};

const wchar_t* const empty_string<wchar_t>::value = L"";

template<typename T>
struct option_exception : std::runtime_error
{
	typedef std::basic_string<T> string_type;
	explicit option_exception(const std::string& msg, const string_type& option_) : std::runtime_error(msg + to_narrow(option_)), opt(option_)
	{
	}
	const std::basic_string<T>& option() const
	{
		return opt;
	}
private:
	string_type opt;
};

std::string to_narrow(const std::string& rhs)
{
	return rhs;
}

std::string to_narrow(const std::wstring& rhs)
{
	return utility::wideToNarrow(rhs);
}

template<typename T>
struct missing_argument : option_exception<T>
{
	explicit missing_argument(const std::string& msg, const string_type& option_) : option_exception(msg, option_)
	{
	}
};

template<typename T>
struct ambiguous_option : option_exception<T>
{
	explicit ambiguous_option(const std::string& msg, const string_type& option_) : option_exception(msg, option_)
	{
	}
};

template<typename T>
struct excess_argument : option_exception<T>
{
	explicit excess_argument(const std::string& msg, const string_type& option_) : option_exception(msg, option_)
	{
	}
};

template<typename T>
struct unknown_option : option_exception<T>
{
	explicit unknown_option(const std::string& msg, const string_type& option_) : option_exception(msg, option_)
	{
	}
};

enum arg_type
{
	no_argument, required_argument, optional_argument
};

enum option_flags
{
	nopermute = 1, // do not permute non-options to the end of argv.  Equivalent to an initial "+" in the options string.
	allargs = 2, // treat non-options as args to option "1".  Equivalent to an initial "-" in the options string.
	longonly = 4, // operate as getopt_long_only
	woptions = 8, // allow -W long-opt-name instead of --long-opt-name.  Equivalent to "W;" in the options string.
	ignore_missing_args = 16 // ignore missing arguments.  Equivalent to an initial ":" in the options string.
};

template<typename T>
struct getopt
{
	typedef std::basic_string<T> string_type;

	struct option
	{
		const T* long_name;
		T short_name;
		arg_type has_arg;
		const T* description;
		option(const T* long_name_, const T short_name_, arg_type arg_spec_, const T* description_) : long_name(long_name_), short_name(short_name_), has_arg(arg_spec_), description(description_)
		{
		}
		option(const option& rhs) : long_name(rhs.long_name), short_name(rhs.short_name), has_arg(rhs.has_arg), description(rhs.description)
		{
		}
		option& operator=(const option& rhs)
		{
			long_name = rhs.long_name;
			short_name = rhs.short_name;
			has_arg = rhs.has_arg;
			description = rhs.description;
			return *this;
		}
	};

	struct result
	{
		typename std::vector<option>::const_iterator chosen_option; // chosen long option
		int optind; // index into parent argv vector
		const T* optarg; // argument associated with option

		result() : optind(1), optarg(0)
		{
		}
	};

	getopt(const std::vector<option>& options_) : nonopt_start(-1), nonopt_end(-1), options(&options_), flags(0)
	{
	}
	getopt(const std::vector<option>& options_, int flags_) : nonopt_start(-1), nonopt_end(-1), options(&options_), flags(flags_)
	{
	}

	int get_opt(result* state, int nargc, T* nargv[])
	{
		return getopt_internal(state, nargc, nargv);
	}

private:
	int nonopt_start; // first non option argument (for permute)
	int nonopt_end;   // first option after non options (for permute)
	int flags;
	const std::vector<option>* options;

	typename std::vector<option>::const_iterator find_option(const T ch)
	{
		for(std::vector<option>::const_iterator it(options->begin()), end(options->end()); it != end; ++it)
		{
			if(it->short_name == ch)
			{
				return it;
			}
		}
		return options->end();
	}

	// Compute the greatest common divisor of a and b.
	template<typename I>
	I gcd(I a, I b)
	{
		I c(a % b);
		while(c != 0)
		{
			a = b;
			b = c;
			c = a % b;
		}
		return b;
	}

	// Exchange the block from nonopt_start to nonopt_end with the block
	// from nonopt_end to opt_end (keeping the same order of arguments
	// in each block).
	void permute_args(int panonopt_start, int panonopt_end, int opt_end, T* nargv[])
	{
		// compute lengths of blocks and number and size of cycles
		const int nnonopts(panonopt_end - panonopt_start);
		const int nopts(opt_end - panonopt_end);
		const int ncycle(gcd(nnonopts, nopts));
		const int cyclelen((opt_end - panonopt_start) / ncycle);

		for(int i(0); i < ncycle; ++i)
		{
			int cstart(panonopt_end + i);
			int pos(cstart);
			for(int j(0); j < cyclelen; ++j)
			{
				if(pos >= panonopt_end)
				{
					pos -= nnonopts;
				}
				else
				{
					pos += nopts;
				}
				std::swap(nargv[pos], nargv[cstart]);
			}
		}
	}

	// parse_long_options -- Parse long options in argc/argv argument vector.
	// Returns -1 if short_too is set and the option does not match long_options.
	int parse_long_options(result* state, T* nargv[], const T* current_argv, int short_too)
	{
		size_t current_argv_len;

		state->optind++;

		const T* has_equal(utility::strchr(current_argv, T('=')));
		if(has_equal != NULL) // argument found (--option=arg)
		{
			current_argv_len = has_equal - current_argv;
			++has_equal;
		}
		else
		{
			current_argv_len = utility::string_length(current_argv);
		}

		typename std::vector<option>::const_iterator match(options->end());
		for(typename std::vector<option>::const_iterator it(options->begin()), end(options->end()); it != end; ++it)
		{
			// find matching long option
			if(utility::string_compare(current_argv, it->long_name, current_argv_len))
			{
				continue;
			}

			if(utility::string_length(it->long_name) == current_argv_len) // exact match
			{
				match = it;
				break;
			}
			
			// If this is a known short option, don't allow
			// a partial match of a single character.
			if(short_too && current_argv_len == 1)
			{
				continue;
			}
			// partial match
			if(match == options->end())
			{
				match = it;
			}
			else
			{
				throw ambiguous_option<T>("ambiguous option: ", string_type(current_argv));
			}
		}
		// option found
		if(match != options->end())
		{
			if(match->has_arg == no_argument && has_equal)
			{
				throw excess_argument<T>("option doesn't take argument: ", string_type(current_argv));
			}
			if(match->has_arg == required_argument || match->has_arg == optional_argument)
			{
				if(has_equal)
				{
					state->optarg = has_equal;
				}
				else if(match->has_arg == required_argument)
				{
					// optional argument doesn't use next nargv
					state->optarg = nargv[state->optind++];
				}
			}
			if((match->has_arg == required_argument) && (state->optarg == NULL) && !(flags & ignore_missing_args))
			{
				throw missing_argument<T>("option requires argument: ", string_type(current_argv));
			}
		}
		// unknown option
		else
		{
			if(short_too)
			{
				--state->optind;
				return -1;
			}
			throw unknown_option<T>("unknown option: ", string_type(current_argv));
		}

		state->chosen_option = match;
		return match->short_name;
	}

	// getopt_internal -- Parse argc/argv argument vector.  Called by user level routines.
	int getopt_internal(result* state, int nargc, T* nargv[])
	{
		const T* place(empty_string<T>::value); // option letter processing
		state->optarg = NULL;
	start:
		// update scanning pointer
		if(state->optind >= nargc) // end of argument vector
		{
			if(nonopt_end != -1)
			{
				// do permutation, if we have to
				permute_args(nonopt_start, nonopt_end, state->optind, nargv);
				state->optind -= nonopt_end - nonopt_start;
			}
			else if(nonopt_start != -1)
			{
				// If we skipped non-options, set optind
				// to the first of them.
				state->optind = nonopt_start;
			}
			nonopt_start = nonopt_end = -1;
			return -1;
		}
		place = nargv[state->optind];
		if(place[0] != T('-') || (place[1] == T('\0') && find_option(T('-')) == options->end()))
		{
			if(flags & allargs)
			{
				// GNU extension:
				// return non-option as argument to option 1
				state->optarg = nargv[state->optind++];
				return INORDER;
			}
			if(flags & nopermute)
			{
				// If no permutation wanted, stop parsing
				// at first non-option.
				return -1;
			}
			// do permutation
			if(nonopt_start == -1)
			{
				nonopt_start = state->optind;
			}
			else if(nonopt_end != -1)
			{
				permute_args(nonopt_start, nonopt_end, state->optind, nargv);
				nonopt_start = state->optind - (nonopt_end - nonopt_start);
				nonopt_end = -1;
			}
			state->optind++;
			// process next argument
			goto start;
		}
		if(nonopt_start != -1 && nonopt_end == -1)
		{
			nonopt_end = state->optind;
		}

		// If we have "-" do nothing, if "--" we are done.
		if(place[1] != T('\0') && *++place == T('-') && place[1] == T('\0'))
		{
			state->optind++;
			// We found an option (--), so if we skipped
			// non-options, we have to permute.
			if(nonopt_end != -1)
			{
				permute_args(nonopt_start, nonopt_end, state->optind, nargv);
				state->optind -= nonopt_end - nonopt_start;
			}
			nonopt_start = nonopt_end = -1;
			return -1;
		}

		// Check long options if:
		//  1) we were passed some
		//  2) the arg is not just "-"
		//  3) either the arg starts with -- or we are getopt_long_only()
		if(place != nargv[state->optind] && (*place == T('-') || (flags & longonly)))
		{
			int short_too(0);
			if(*place == T('-'))
			{
				place++; // --foo long option
			}
			else if(*place == T('W') || (*place != T(':') && find_option(*place) != options->end()))
			{
				short_too = 1; // could be short option too
			}

			int optchar(parse_long_options(state, nargv, place, short_too));
			if(optchar != -1)
			{
				return optchar;
			}
		}

		int optchar((int)*place++);
		// W is special because we won't find it in the options array.
		if(optchar == T('W') && (flags & woptions))
		{
			// -W long-option
			if(*place) // no space
			{
				// NOTHING
			}
			else if(++state->optind >= nargc) // no arg
			{
				throw missing_argument<T>("option requires argument: ", string_type() + static_cast<T>(optchar));
			}
			else // white space
			{
				place = nargv[state->optind];
			}
			optchar = parse_long_options(state, nargv, place, 0);
			return optchar;
		}

		typename std::vector<option>::const_iterator oli(find_option(optchar)); // relevant option
		if(optchar == (int)T(':') ||
		  (optchar == (int)T('-') && *place != T('\0')) ||
		   oli == options->end())
		{
			// If the user specified "-" and  '-' isn't listed in
			// options, return -1 (non-option) as per POSIX.
			// Otherwise, it is an unknown option character (or ':').
			if(optchar == (int)T('-') && *place == T('\0'))
			{
				return -1;
			}
			if(!*place)
			{
				state->optind++;
			}
			throw unknown_option<T>("unknown option: ", string_type() + static_cast<T>(optchar));
		}
		if(oli->has_arg == no_argument) // doesn't take argument
		{
			if(!*place)
			{
				state->optind++;
			}
		}
		else // takes (optional) argument
		{
			state->optarg = NULL;
			if(*place) // no white space
			{
				state->optarg = place;
			}
			else if(oli->has_arg != optional_argument) // arg not optional
			{
				if(++state->optind >= nargc) // no arg
				{
					throw missing_argument<T>("option requires argument: ", string_type() + static_cast<T>(optchar));
				}
				else
				{
					state->optarg = nargv[state->optind];
				}
			}
			else if(flags & nopermute)
			{
				// If permutation is disabled, we can accept an
				// optional arg separated by whitespace.
				if(state->optind + 1 < nargc)
				{
					state->optarg = nargv[++state->optind];
				}
			}
			++state->optind;
		}
		// dump back option letter
		return optchar;
	}
};

#if 0
#include "getopt.h"

void getopt_test(int argc, wchar_t* argv[])
{
	static getopt<wchar_t>::option long_options[] =
	{
		getopt<wchar_t>::option(L"help", L'?', no_argument, L"Print this message"),
		getopt<wchar_t>::option(L"buffer-size", L'b', required_argument, L"Set total buffer size file reading"),
		getopt<wchar_t>::option(L"source", L'\0', required_argument, L"Source directory"),
		getopt<wchar_t>::option(L"pattern", L'p', required_argument, L"Filename wildcard pattern"),
		getopt<wchar_t>::option(L"epattern", L'e', required_argument, L"Filename regular expression"),
		getopt<wchar_t>::option(L"optional", L'o', optional_argument, L"Test option with optional argument"),
		getopt<wchar_t>::option(0, 0, no_argument, 0)
	};

	std::vector<getopt<wchar_t>::option> opts;
	opts.push_back(getopt<wchar_t>::option(L"help", L'?', no_argument, L"Print this message"));
	opts.push_back(getopt<wchar_t>::option(L"buffer-size", L'b', required_argument, L"Set total buffer size file reading"));
	opts.push_back(getopt<wchar_t>::option(L"source", L'\0', required_argument, L"Source directory"));
	opts.push_back(getopt<wchar_t>::option(L"pattern", L'p', required_argument, L"Filename wildcard pattern"));
	opts.push_back(getopt<wchar_t>::option(L"epattern", L'e', required_argument, L"Filename regular expression"));
	opts.push_back(getopt<wchar_t>::option(L"optional", L'o', optional_argument, L"Test option with optional argument"));
	opts.push_back(getopt<wchar_t>::option(L"short-one", L'q', no_argument, L"First short argument"));
	opts.push_back(getopt<wchar_t>::option(L"short-two", L'r', no_argument, L"Second short argument"));

	getopt<wchar_t> options(opts, allargs | woptions | longonly);
	getopt<wchar_t>::result result;
	while(true)
	{
		switch(int c = options.get_opt(&result, argc, argv))
		{
		case -1:
			goto end;
		case 0:
			wprintf(L"option %s", result.chosen_option->long_name);
			if(result.optarg)
			{
				wprintf(L" with value `%s'", result.optarg);
			}
			wprintf(L"\n");
			break;
		case 1:
			wprintf(L"non-option `%s'\n", result.optarg);
			break;
		case '?':
			wprintf(L"option ?\n");
			break;
		case 'b':
			wprintf(L"option b with value `%s'\n", result.optarg);
			break;
		case 's':
			wprintf(L"option s with value `%s'\n", result.optarg);
			break;
		case 'q':
			wprintf(L"option q\n");
			break;
		case 'r':
			wprintf(L"option r\n");
			break;
		case 'p':
			wprintf(L"option p with value `%s'\n", result.optarg);
			break;
		case 'e':
			wprintf(L"option e with value `%s'\n", result.optarg);
			break;
		case 'o':
			wprintf(L"option o");
			if(result.optarg)
			{
				wprintf(L" with value `%s'", result.optarg);
			}
			wprintf(L"\n");
			break;
		default:
			wprintf(L"?? getopt returned character code 0%o ??\n", c);
		}
	}
end:
	if(result.optind < argc)
	{
		wprintf(L"non-option ARGV-elements: ");
		while(result.optind < argc)
		{
			wprintf(L"%s ", argv[result.optind++]);
		}
		wprintf(L"\n");
	}
}
#endif

#endif
