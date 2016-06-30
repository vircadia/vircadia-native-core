#
#  FindQuaZip.cmake
#
#  Once done this will define
#
#  QUAZIP_FOUND - system found QuaZip
#  QUAZIP_INCLUDE_DIRS - the QuaZip include directory
#  QUAZIP_LIBRARIES - link to this to use QuaZip
#
#  Created on 2015-8-1 by Seth Alves
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("quazip")

find_path(QUAZIP_INCLUDE_DIRS quazip.h PATH_SUFFIXES include HINTS ${QUAZIP_SEARCH_DIRS})

find_library(QUAZIP_LIBRARY_DEBUG NAMES QUAZIP QUAZIP_LIB PATH_SUFFIXES lib/Debug HINTS ${QUAZIP_SEARCH_DIRS})
find_library(QUAZIP_LIBRARY_RELEASE NAMES QUAZIP QUAZIP_LIB PATH_SUFFIXES lib/Release lib HINTS ${QUAZIP_SEARCH_DIRS})

include(SelectLibraryConfigurations)
select_library_configurations(QUAZIP)

set(QUAZIP_LIBRARIES ${QUAZIP_LIBRARY})

find_package_handle_standard_args(QUAZIP "Could NOT find QuaZip, try to set the path to QuaZip root folder in the system variable QUAZIP_ROOT_DIR or create a directory quazip in HIFI_LIB_DIR and paste the necessary files there"
 QUAZIP_INCLUDE_DIRS QUAZIP_LIBRARIES)

mark_as_advanced(QUAZIP_INCLUDE_DIRS QUAZIP_LIBRARIES QUAZIP_SEARCH_DIRS)