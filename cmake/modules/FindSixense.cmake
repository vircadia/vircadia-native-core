# 
#  FindSixense.cmake 
# 
#  Try to find the Sixense controller library
#
#  You must provide a SIXENSE_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  SIXENSE_FOUND - system found Sixense
#  SIXENSE_INCLUDE_DIRS - the Sixense include directory
#  SIXENSE_LIBRARIES - Link this to use Sixense
#
#  Created on 11/15/2013 by Andrzej Kapolka
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("SIXENSE" "sixense")

find_path(SIXENSE_INCLUDE_DIRS sixense.h PATH_SUFFIXES include HINTS ${SIXENSE_SEARCH_DIRS})

if (APPLE)
  find_library(SIXENSE_LIBRARY_RELEASE lib/osx_x64/release_dll/libsixense_x64.dylib HINTS ${SIXENSE_SEARCH_DIRS})
  find_library(SIXENSE_LIBRARY_DEBUG lib/osx_x64/debug_dll/libsixensed_x64.dylib HINTS ${SIXENSE_SEARCH_DIRS})
elseif (UNIX)
  find_library(SIXENSE_LIBRARY_RELEASE lib/linux_x64/release/libsixense_x64.so HINTS ${SIXENSE_SEARCH_DIRS})
  # find_library(SIXENSE_LIBRARY_DEBUG lib/linux_x64/debug/libsixensed_x64.so HINTS ${SIXENSE_SEARCH_DIRS})
elseif (WIN32)
  find_library(SIXENSE_LIBRARY_RELEASE lib/win32/release_dll/sixense.lib HINTS ${SIXENSE_SEARCH_DIRS})
  find_library(SIXENSE_LIBRARY_DEBUG lib/win32/debug_dll/sixensed.lib HINTS ${SIXENSE_SEARCH_DIRS})
endif ()

include(SelectLibraryConfigurations)
select_library_configurations(SIXENSE)

set(SIXENSE_LIBRARIES "${SIXENSE_LIBRARY}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SIXENSE DEFAULT_MSG SIXENSE_INCLUDE_DIRS SIXENSE_LIBRARIES)

mark_as_advanced(SIXENSE_LIBRARIES SIXENSE_INCLUDE_DIRS)
