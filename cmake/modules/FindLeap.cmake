# - Try to find the Leap library
#
#  You must provide a LEAP_ROOT_DIR which contains Lib and Include directories
#
#  Once done this will define
#
#  LEAP_FOUND - system found Leap
#  LEAP_INCLUDE_DIRS - the Leap include directory
#  LEAP_LIBRARIES - Link this to use Leap
#
#  Created on 6/21/2013 by Eric Johnston, 
#         adapted from FindLibOVR.cmake by Stephen Birarda
#  Copyright (c) 2013 High Fidelity
#

if (LEAP_LIBRARIES AND LEAP_INCLUDE_DIRS)
  # in cache already
  set(LEAP_FOUND TRUE)
else (LEAP_LIBRARIES AND LEAP_INCLUDE_DIRS)
  find_path(LEAP_INCLUDE_DIRS Leap.h ${LEAP_ROOT_DIR}/include)

  if (LEAP_INCLUDE_DIRS)
    ### If we found the real header, get the library as well.
    if (APPLE)
      find_library(LEAP_LIBRARIES libLeap.dylib ${LEAP_ROOT_DIR}/lib/libc++/)
    elseif (WIN32)
      find_library(LEAP_LIBRARIES libLeap.dylib ${LEAP_ROOT_DIR}/lib/libc++/)
    endif ()
  else ()
    ### If we didn't find the real header, just use the stub header, and no library.
    find_path(LEAP_INCLUDE_DIRS Leap.h ${LEAP_ROOT_DIR}/stubs/include)
  endif ()

# If we're using the Leap stubs, there's only a header, no lib.
  if (LEAP_LIBRARIES AND LEAP_INCLUDE_DIRS)
     set(LEAP_FOUND TRUE)
  endif (LEAP_LIBRARIES AND LEAP_INCLUDE_DIRS)

  if (LEAP_FOUND)
    if (NOT Leap_FIND_QUIETLY)
      message(STATUS "Found Leap: ${LEAP_LIBRARIES}")
    endif (NOT Leap_FIND_QUIETLY)
  else (LEAP_FOUND)
    if (Leap_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find Leap")
    endif (Leap_FIND_REQUIRED)
  endif (LEAP_FOUND)

  # show the LEAP_INCLUDE_DIRS and LEAP_LIBRARIES variables only in the advanced view
  mark_as_advanced(LEAP_INCLUDE_DIRS LEAP_LIBRARIES)

endif (LEAP_LIBRARIES AND LEAP_INCLUDE_DIRS)