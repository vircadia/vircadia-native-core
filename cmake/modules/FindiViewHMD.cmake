#
#  FindiViewHMD.cmake
#
#  Try to find the SMI iViewHMD eye tracker library
#
#  You must provide a IVIEWHMD_ROOT_DIR which contains 3rdParty, include, and libs directories
#
#  Once done this will define
#
#  IVIEWHMD_FOUND - system found iViewHMD
#  IVIEWHMD_INCLUDE_DIRS - the iViewHMD include directory
#  IVIEWHMD_LIBRARIES - link this to use iViewHMD
#
#  Created on 27 Jul 2015 by David Rowe
#  Copyright 2015 High Fidelity, Inc.
#

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("iViewHMD")

find_path(IVIEWHMD_INCLUDE_DIRS iViewHMDAPI.h PATH_SUFFIXES include HINTS ${IVIEWHMD_SEARCH_DIRS})
find_library(IVIEWHMD_LIBRARIES NAMES iViewHMDAPI PATH_SUFFIXES libs HINTS ${IVIEWHMD_SEARCH_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(iViewHMD DEFAULT_MSG IVIEWHMD_INCLUDE_DIRS IVIEWHMD_LIBRARIES)

mark_as_advanced(IVIEWHMD_INCLUDE_DIRS IVIEWHMD_LIBRARIES IVIEWHMD_SEARCH_DIRS)
