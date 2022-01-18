#
#  FindCrashpad.cmake
#
#  Try to find Crashpad libraries and include path.
#  Once done this will define
#
#  CRASHPAD_FOUND
#  CRASHPAD_INCLUDE_DIRS
#  CRASHPAD_LIBRARY
#  CRASHPAD_BASE_LIBRARY
#  CRASHPAD_UTIL_LIBRARY
#
#  Created on 01/19/2018 by Clement Brisset
#  Copyright 2018 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("crashpad")

find_path(CRASHPAD_INCLUDE_DIRS base/macros.h PATH_SUFFIXES include HINTS ${CRASHPAD_SEARCH_DIRS})

find_library(CRASHPAD_LIBRARY_RELEASE crashpad PATH_SUFFIXES "Release_x64/lib_MD" HINTS ${CRASHPAD_SEARCH_DIRS})
find_library(CRASHPAD_BASE_LIBRARY_RELEASE base PATH_SUFFIXES "Release_x64/lib_MD" HINTS ${CRASHPAD_SEARCH_DIRS})
find_library(CRASHPAD_UTIL_LIBRARY_RELEASE util PATH_SUFFIXES "Release_x64/lib_MD" HINTS ${CRASHPAD_SEARCH_DIRS})

find_library(CRASHPAD_LIBRARY_DEBUG crashpad PATH_SUFFIXES "Debug_x64/lib_MD" HINTS ${CRASHPAD_SEARCH_DIRS})
find_library(CRASHPAD_BASE_LIBRARY_DEBUG base PATH_SUFFIXES "Debug_x64/lib_MD" HINTS ${CRASHPAD_SEARCH_DIRS})
find_library(CRASHPAD_UTIL_LIBRARY_DEBUG util PATH_SUFFIXES "Debug_x64/lib_MD" HINTS ${CRASHPAD_SEARCH_DIRS})

find_file(CRASHPAD_HANDLER_EXE_PATH NAME "crashpad_handler.exe" PATH_SUFFIXES "Release_x64" HINTS ${CRASHPAD_SEARCH_DIRS})

include(SelectLibraryConfigurations)
select_library_configurations(CRASHPAD)
select_library_configurations(CRASHPAD_BASE)
select_library_configurations(CRASHPAD_UTIL)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Crashpad DEFAULT_MSG CRASHPAD_INCLUDE_DIRS CRASHPAD_LIBRARY CRASHPAD_BASE_LIBRARY CRASHPAD_UTIL_LIBRARY)
