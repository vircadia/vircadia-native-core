#
#  FindSoxr.cmake
# 
#  Try to find the libsoxr resampling library
#
#  You can provide a LIBSOXR_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  SOXR_FOUND - system found libsoxr
#  SOXR_INCLUDE_DIRS - the libsoxr include directory
#  SOXR_LIBRARIES - link to this to use libsoxr
#
#  Created on 1/22/2015 by Stephen Birarda
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("soxr")

find_path(SOXR_INCLUDE_DIRS soxr.h PATH_SUFFIXES include HINTS ${SOXR_SEARCH_DIRS})
find_library(SOXR_LIBRARIES NAMES soxr PATH_SUFFIXES lib HINTS ${SOXR_SEARCH_DIRS})

if (WIN32)
  find_path(SOXR_DLL_PATH soxr.dll PATH_SUFFIXES bin HINTS ${SOXR_SEARCH_DIRS})
  add_path_to_lib_paths(${SOXR_DLL_PATH})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SOXR DEFAULT_MSG SOXR_INCLUDE_DIRS SOXR_LIBRARIES)

mark_as_advanced(SOXR_INCLUDE_DIRS SOXR_LIBRARIES SOXR_SEARCH_DIRS)