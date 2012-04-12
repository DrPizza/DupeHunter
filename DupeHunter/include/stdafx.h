// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define NOMINMAX
#define STRICT
#define ISOLATION_AWARE_ENABLED 1
#pragma warning(disable:4995)
#pragma warning(disable:4996)

#include <windows.h>
#include <Shellapi.h>

#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <functional>
#include <memory>

#include <boost/regex.hpp>
#include <boost/program_options.hpp>

#include <utility/scopeguard.hpp>
