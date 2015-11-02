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

if (WIN32)

    include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
    hifi_library_search_hints("iViewHMD")

    find_path(IVIEWHMD_INCLUDE_DIRS iViewHMDAPI.h PATH_SUFFIXES include HINTS ${IVIEWHMD_SEARCH_DIRS})
    find_library(IVIEWHMD_LIBRARIES NAMES iViewHMDAPI PATH_SUFFIXES libs/x86 HINTS ${IVIEWHMD_SEARCH_DIRS})
    find_path(IVIEWHMD_API_DLL_PATH iViewHMDAPI.dll PATH_SUFFIXES libs/x86 HINTS ${IVIEWHMD_SEARCH_DIRS})
    list(APPEND IVIEWHMD_REQUIREMENTS IVIEWHMD_INCLUDE_DIRS IVIEWHMD_LIBRARIES IVIEWHMD_API_DLL_PATH)

    find_path(IVIEWHMD_DLL_PATH_3RD_PARTY libiViewNG.dll PATH_SUFFIXES 3rdParty HINTS ${IVIEWHMD_SEARCH_DIRS})
    list(APPEND IVIEWHMD_REQUIREMENTS IVIEWHMD_DLL_PATH_3RD_PARTY)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(IVIEWHMD DEFAULT_MSG ${IVIEWHMD_REQUIREMENTS})

    add_paths_to_fixup_libs(${IVIEWHMD_API_DLL_PATH} ${IVIEWHMD_DLL_PATH_3RD_PARTY})

    mark_as_advanced(IVIEWHMD_INCLUDE_DIRS IVIEWHMD_LIBRARIES IVIEWHMD_SEARCH_DIRS)

endif()
