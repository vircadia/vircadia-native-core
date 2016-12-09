#
#  FindKinect.cmake
#
#  Try to find the Perception Kinect SDK
#
#  You must provide a KINECT_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  KINECT_FOUND - system found Kinect SDK
#  KINECT_INCLUDE_DIRS - the Kinect SDK include directory
#  KINECT_LIBRARIES - Link this to use Kinect
#
#  Created by Brad Hefta-Gaub on 2016/12/7
#  Copyright 2016 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("kinect")

find_path(KINECT_INCLUDE_DIRS Kinect.h PATH_SUFFIXES inc HINTS $ENV{KINECT_ROOT_DIR})

if (WIN32)

  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
      set(ARCH_DIR "x64")
  else()
      set(ARCH_DIR "x86")
  endif()

  find_library(
        KINECT_LIBRARY_RELEASE Kinect20 
        PATH_SUFFIXES "Libs/${ARCH_DIR}" "lib" 
        HINTS ${KINECT_SEARCH_DIRS}
        PATH "C:/Program Files/Microsoft SDKs/Kinect/v2.0_1409")

  set(KINECT_LIBRARIES ${KINECT_LIBRARY})

  # DLL not needed yet??
  #find_path(KINECT_DLL_PATH Kinect20.Face.dll PATH_SUFFIXES "bin" HINTS ${KINECT_SEARCH_DIRS})


endif ()

include(SelectLibraryConfigurations)
select_library_configurations(KINECT)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(KINECT DEFAULT_MSG KINECT_INCLUDE_DIRS KINECT_LIBRARY)

# DLLs not needed yet
#if (WIN32)
#  add_paths_to_fixup_libs(${KINECT_DLL_PATH})
#endif ()

mark_as_advanced(KINECT_INCLUDE_DIRS KINECT_LIBRARIES KINECT_SEARCH_DIRS)
