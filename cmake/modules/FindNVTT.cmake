#
#  FindNVTT.cmake
#
#  Try to find NVIDIA texture tools library and include path.
#  Once done this will define
#
#  NVTT_FOUND
#  NVTT_INCLUDE_DIRS
#  NVTT_LIBRARY
#
#  Created on 4/14/2017 by Stephen Birarda
#  Copyright 2017 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("nvtt")

find_path(NVTT_INCLUDE_DIRS nvtt/nvtt.h PATH_SUFFIXES include HINTS ${NVTT_SEARCH_DIRS})

find_library(NVTT_LIBRARY nvtt PATH_SUFFIXES "lib/static" HINTS ${NVTT_SEARCH_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NVTT DEFAULT_MSG NVTT_INCLUDE_DIRS NVTT_LIBRARY)
