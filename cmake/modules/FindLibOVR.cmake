# - Try to find the LibOVR library to use the Oculus
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
#  Copyright (c) 2013 High Fidelity
#

if (LIBOVR_LIBRARIES AND LIBOVR_INCLUDE_DIRS)
  # in cache already
  set(LIBOVR_FOUND TRUE)
else (LIBOVR_LIBRARIES AND LIBOVR_INCLUDE_DIRS)
  set(LIBOVR_SEARCH_DIRS "${LIBOVR_ROOT_DIR}" "$ENV{HIFI_LIB_DIR}/sixense")
  
  find_path(LIBOVR_INCLUDE_DIRS Include/OVR.h HINTS "${LIBOVR_SEARCH_DIRS}")
  
  if (APPLE)
    find_library(LIBOVR_LIBRARIES "Lib/MacOS/${CMAKE_BUILD_TYPE}/libovr.a" HINTS "${LIBOVR_SEARCH_PATHS}")
  elseif (UNIX)
    find_library(UDEV_LIBRARY libudev.a /usr/lib/x86_64-linux-gnu/)
    find_library(XINERAMA_LIBRARY libXinerama.a /usr/lib/x86_64-linux-gnu/)
    
    if (CMAKE_CL_64)
      set(LINUX_ARCH_DIR "i386")
    else()
      set(LINUX_ARCH_DIR "x86_64")
    endif()
    
    find_library(OVR_LIBRARY "Lib/Linux/${CMAKE_BUILD_TYPE}/${LINUX_ARCH_DIR}/libovr.a" HINTS "${LIBOVR_SEARCH_PATHS}")
    if (UDEV_LIBRARY AND XINERAMA_LIBRARY AND OVR_LIBRARY)
      set(LIBOVR_LIBRARIES "${OVR_LIBRARY};${UDEV_LIBRARY};${XINERAMA_LIBRARY}" CACHE INTERNAL "Oculus libraries")
    endif (UDEV_LIBRARY AND XINERAMA_LIBRARY AND OVR_LIBRARY)
  elseif (WIN32)
    if (CMAKE_CL_64)
      set(WINDOWS_ARCH_DIR "Win32")
      
      if (CMAKE_BUILD_TYPE MATCHES DEBUG)
        set(WINDOWS_LIBOVR_NAME "libovrd.lib")
      else()
        set(WINDOWS_LIBOVR_NAME "libovr.lib")
      endif()
    else()
      set(WINDOWS_ARCH_DIR "x64")
      
      if (CMAKE_BUILD_TYPE MATCHES DEBUG)
        set(WINDOWS_LIBOVR_NAME "libovr64d.lib")
      else()
        set(WINDOWS_LIBOVR_NAME "libovr64.lib")
      endif()
    endif()
    
    find_library(LIBOVR_LIBRARIES "Lib/${WINDOWS_ARCH_DIR}/${LIBOVR_NAME}" HINTS "${LIBOVR_SEARCH_DIRS}")
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
