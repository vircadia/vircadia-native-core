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
    find_path(IVIEWHMD_API_DLL_PATH iViewHMDAPI.dll PATH_SUFFIXES libs HINTS ${IVIEWHMD_SEARCH_DIRS})
    list(APPEND IVIEWHMD_REQUIREMENTS IVIEWHMD_INCLUDE_DIRS IVIEWHMD_LIBRARIES IVIEWHMD_API_DLL_PATH)

    set(IVIEWHMD_DLLS
        avcodec-53.dll
        avformat-53.dll
        avutil-51.dll
        libboost_filesystem-mgw45-mt-1_49.dll
        libboost_system-mgw45-mt-1_49.dll
        libboost_thread-mgw45-mt-1_49.dll
        libgcc_s_dw2-1.dll
        libiViewNG-LibCore.dll
        libopencv_calib3d244.dll
        libopencv_core244.dll
        libopencv_features2d244.dll
        libopencv_flann244.dll
        libopencv_highgui244.dll
        libopencv_imgproc244.dll
        libopencv_legacy244.dll
        libopencv_ml244.dll
        libopencv_video244.dll
        libstdc++-6.dll
        opencv_core220.dll
        opencv_highgui220.dll
        opencv_imgproc220.dll
        swscale-2.dll
    )

    foreach(IVIEWHMD_DLL ${IVIEWHMD_DLLS})
        find_path(IVIEWHMD_DLL_PATH ${IVIEWHMD_DLL} PATH_SUFFIXES 3rdParty HINTS ${IVIEWHMD_SEARCH_DIRS})
        list(APPEND IVIEWHMD_REQUIREMENTS IVIEWHMD_DLL_PATH)
        list(APPEND IVIEWHMD_DLL_PATHS ${IVIEWHMD_DLL_PATH})
    endforeach()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(IVIEWHMD DEFAULT_MSG ${IVIEWHMD_REQUIREMENTS})

    add_paths_to_fixup_libs(${IVIEWHMD_API_DLL_PATH} ${IVIEWHMD_DLL_PATHS})

    mark_as_advanced(IVIEWHMD_INCLUDE_DIRS IVIEWHMD_LIBRARIES IVIEWHMD_SEARCH_DIRS)

endif()
