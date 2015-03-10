#  FindGVerb.cmake
# 
#  Try to find the Gverb library.
#
#  You must provide a GVERB_ROOT_DIR which contains src and include directories
#
#  Once done this will define
#
#  GVERB_FOUND - system found Gverb
#  GVERB_INCLUDE_DIRS - the Gverb include directory
#
#  Copyright 2014 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("gverb")

find_path(GVERB_INCLUDE_DIRS gverb.h PATH_SUFFIXES include HINTS ${GVERB_SEARCH_DIRS})
find_library(GVERB_LIBRARIES gverb PATH_SUFFIXES lib HINTS ${GVERB_SEARCH_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gverb DEFAULT_MSG GVERB_INCLUDE_DIRS GVERB_LIBRARIES)