# 
#  FindLibOVR.cmake
# 
#  Try to find the LibOVR library to use the Oculus
#
#  You must provide a LIBOVR_ROOT_DIR which contains Lib and Include directories
#
#  Once done this will define
#
#  LIBOVR_FOUND - system found LibOVR
#  LIBOVR_INCLUDE_DIRS - the LibOVR include directory
#  LIBOVR_LIBRARIES - Link this to use LibOVR
#
#  Created on 5/9/2013 by Stephen Birarda
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

if (LIBOVR_LIBRARIES AND LIBOVR_INCLUDE_DIRS)
  # in cache already
  set(LIBOVR_FOUND TRUE)
else (LIBOVR_LIBRARIES AND LIBOVR_INCLUDE_DIRS)
  set(LIBOVR_SEARCH_DIRS "${LIBOVR_ROOT_DIR}" "$ENV{HIFI_LIB_DIR}/oculus")
  
  find_path(LIBOVR_INCLUDE_DIRS OVR.h PATH_SUFFIXES Include HINTS ${LIBOVR_SEARCH_DIRS})
  
  if (APPLE)
    find_library(LIBOVR_LIBRARY_DEBUG "Lib/MacOS/Debug/libovr.a" HINTS ${LIBOVR_SEARCH_DIRS})
    find_library(LIBOVR_LIBRARY_RELEASE "Lib/MacOS/Release/libovr.a" HINTS ${LIBOVR_SEARCH_DIRS})
  elseif (UNIX)
    find_library(UDEV_LIBRARY_RELEASE libudev.a /usr/lib/x86_64-linux-gnu/)
    find_library(XINERAMA_LIBRARY_RELEASE libXinerama.a /usr/lib/x86_64-linux-gnu/)
    
    if (CMAKE_CL_64)
      set(LINUX_ARCH_DIR "i386")
    else()
      set(LINUX_ARCH_DIR "x86_64")
    endif()
    
    find_library(LIBOVR_LIBRARY_DEBUG "Lib/Linux/Debug/${LINUX_ARCH_DIR}/libovr.a" HINTS ${LIBOVR_SEARCH_DIRS})
    find_library(LIBOVR_LIBRARY_RELEASE "Lib/Linux/Release/${LINUX_ARCH_DIR}/libovr.a" HINTS ${LIBOVR_SEARCH_DIRS})
    
    if (UDEV_LIBRARY AND XINERAMA_LIBRARY AND OVR_LIBRARY)
      set(LIBOVR_LIBRARIES "${OVR_LIBRARY};${UDEV_LIBRARY};${XINERAMA_LIBRARY}" CACHE INTERNAL "Oculus libraries")
    endif (UDEV_LIBRARY AND XINERAMA_LIBRARY AND OVR_LIBRARY)
  elseif (WIN32)   
    find_library(LIBOVR_LIBRARY_DEBUG "Lib/Win32/libovrd.lib" HINTS ${LIBOVR_SEARCH_DIRS})   
    find_library(LIBOVR_LIBRARY_RELEASE "Lib/Win32/libovr.lib" HINTS ${LIBOVR_SEARCH_DIRS})
  endif ()

  include(SelectLibraryConfigurations)
  select_library_configurations(LIBOVR)
  
  if (UNIX)
    select_library_configurations(UDEV)
    select_library_configurations(XINERAMA)
    
    set(${LIBOVR_LIBRARIES} ${LIBOVR_LIBRARIES} ${UDEV_LIBRARY} ${XINERAMA_LIBRARY})
  endif ()
  
  if (LIBOVR_INCLUDE_DIRS AND LIBOVR_LIBRARIES)
     set(LIBOVR_FOUND TRUE)
  endif (LIBOVR_INCLUDE_DIRS AND LIBOVR_LIBRARIES)
 
  if (LIBOVR_FOUND)
    if (NOT LibOVR_FIND_QUIETLY)
      message(STATUS "Found LibOVR: ${LIBOVR_LIBRARIES}")
    endif (NOT LibOVR_FIND_QUIETLY)
  else (LIBOVR_FOUND)
    if (LibOVR_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find LibOVR")
    endif (LibOVR_FIND_REQUIRED)
  endif (LIBOVR_FOUND)

  # show the LIBOVR_INCLUDE_DIRS and LIBOVR_LIBRARIES variables only in the advanced view
  mark_as_advanced(LIBOVR_INCLUDE_DIRS LIBOVR_LIBRARIES)

endif (LIBOVR_LIBRARIES AND LIBOVR_INCLUDE_DIRS)
