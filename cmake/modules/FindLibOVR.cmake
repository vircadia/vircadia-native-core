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
  find_path(LIBOVR_INCLUDE_DIRS OVR.h ${LIBOVR_ROOT_DIR}/Include)

  if (APPLE)
    find_library(LIBOVR_LIBRARIES libovr.a ${LIBOVR_ROOT_DIR}/Lib/MacOS/)
  else (WIN32)
    find_library(LIBOVR_LIBRARIES libovr.lib ${LIBOVR_ROOT_DIR}/Lib/Win32/)
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