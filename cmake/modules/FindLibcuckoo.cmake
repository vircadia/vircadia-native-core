#  FindLibCuckoo.cmake
# 
#  Try to find libcuckoo.
#
#  You can provide a LIBCUCKOO_ROOT_DIR which contains src and include directories
#
#  Once done this will define
#
#  LIBCUCKOO_FOUND - system found libcuckoo
#  LIBCUCKOO_INCLUDE_DIRS - the libcuckoo include directory
#
#  Created on 5/11/2014 by Stephen Birarda
#  Copyright 2014 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("libcuckoo")

find_path(LIBCUCKOO_INCLUDE_DIRS libcuckoo/cuckoohash_map.hh PATH_SUFFIXES include HINTS ${LIBCUCKOO_SEARCH_DIRS})

find_library(CITYHASH_LIBRARY_RELEASE NAME cityhash PATH_SUFFIXES lib HINTS ${LIBCUCKOO_SEARCH_DIRS})
find_library(CITYHASH_LIBRARY_DEBUG NAME cityhash PATH_SUFFIXES lib HINTS ${LIBCUCKOO_SEARCH_DIRS})

include(SelectLibraryConfigurations)
select_library_configurations(CITYHASH)

set(LIBCUCKOO_LIBRARIES ${CITYHASH_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  libcuckoo
  "Could NOT find libcuckoo. Read libraries/networking/externals/libcuckoo/readme.txt" 
  LIBCUCKOO_INCLUDE_DIRS
  LIBCUCKOO_LIBRARIES
)