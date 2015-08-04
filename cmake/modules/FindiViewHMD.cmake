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

    find_library(IVIEWHMD_LIBRARIES NAMES iViewHMDAPI PATH_SUFFIXES libs HINTS ${IVIEWHMD_SEARCH_DIRS})
    find_path(IVIEWHMD_DLL_PATH iViewHMDAPI.dll PATH_SUFFIXES libs HINTS ${IVIEWHMD_SEARCH_DIRS})

    find_path(LIBIVIEWNG_LIBCORE_DLL_PATH libiViewNG-LibCore.dll PATH_SUFFIXES 3rdParty HINTS ${IVIEWHMD_SEARCH_DIRS})
    find_path(OPENCV_HIGHGUI220_DLL_PATH opencv_highgui220.dll PATH_SUFFIXES 3rdParty HINTS ${IVIEWHMD_SEARCH_DIRS})
    find_path(OPENCV_CORE200_DLL_PATH opencv_core220.dll PATH_SUFFIXES 3rdParty HINTS ${IVIEWHMD_SEARCH_DIRS})
    find_path(OPENCV_IMGPROC220_DLL_PATH opencv_imgproc220.dll PATH_SUFFIXES 3rdParty HINTS ${IVIEWHMD_SEARCH_DIRS})
    
    list(APPEND IVIEWHMD_REQUIREMENTS IVIEWHMD_INCLUDE_DIRS IVIEWHMD_LIBRARIES IVIEWHMD_DLL_PATH)
    list(APPEND IVIEWHMD_REQUIREMENTS LIBIVIEWNG_LIBCORE_DLL_PATH OPENCV_HIGHGUI220_DLL_PATH OPENCV_CORE200_DLL_PATH OPENCV_IMGPROC220_DLL_PATH)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(IVIEWHMD DEFAULT_MSG ${IVIEWHMD_REQUIREMENTS})

    add_paths_to_fixup_libs(${IVIEWHMD_DLL_PATH} ${LIBIVIEWNG_LIBCORE_DLL_PATH} ${OPENCV_HIGHGUI220_DLL_PATH} ${OPENCV_CORE200_DLL_PATH} ${OPENCV_IMGPROC220_DLL_PATH})

    mark_as_advanced(IVIEWHMD_INCLUDE_DIRS IVIEWHMD_LIBRARIES IVIEWHMD_SEARCH_DIRS)

endif ()
