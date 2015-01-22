#
#  FindLibsoxr.cmake
# 
#  Try to find the libsoxr resampling library
#
#  You can provide a LIBSOXR_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  LIBSOXR_FOUND - system found libsoxr
#  LIBSOXR_INCLUDE_DIRS - the libsoxr include directory
#  LIBSOXR_LIBRARIES - link to this to use libsoxr
#
#  Created on 1/22/2015 by Stephen Birarda
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("libsoxr")

find_path(LIBSOXR_INCLUDE_DIRS soxr.h PATH_SUFFIXES include HINTS ${LIBSOXR_SEARCH_DIRS})
find_library(LIBSOXR_LIBRARIES NAMES soxr PATH_SUFFIXES lib HINTS ${LIBSOXR_SEARCH_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libsoxr DEFAULT_MSG LIBSOXR_INCLUDE_DIRS LIBSOXR_LIBRARIES)

mark_as_advanced(LIBSOXR_INCLUDE_DIRS LIBSOXR_LIBRARIES LIBSOXR_SEARCH_DIRS)