#  Try to find the Freenect library to read from Kinect
#
#  You must provide a FREENECT_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  FREENECT_FOUND - system found Freenect
#  FREENECT_INCLUDE_DIRS - the Freenect include directory
#  FREENECT_LIBRARIES - Link this to use Freenect
#
#  Created on 6/25/2013 by Andrzej Kapolka
#  Copyright (c) 2013 High Fidelity
#

if (FREENECT_LIBRARIES AND FREENECT_INCLUDE_DIRS)
  # in cache already
  set(FREENECT_FOUND TRUE)
else (FREENECT_LIBRARIES AND FREENECT_INCLUDE_DIRS)
  find_path(FREENECT_INCLUDE_DIRS libfreenect.h ${FREENECT_ROOT_DIR}/include)
  
  if (APPLE)
    find_library(FREENECT_LIBRARIES libfreenect.a ${FREENECT_ROOT_DIR}/lib/MacOS/)
  elseif (UNIX)
    find_library(FREENECT_LIBRARIES libfreenect.a ${FREENECT_ROOT_DIR}/lib/UNIX/)
  endif ()
  
  if (FREENECT_INCLUDE_DIRS AND FREENECT_LIBRARIES)
     set(FREENECT_FOUND TRUE)
  endif (FREENECT_INCLUDE_DIRS AND FREENECT_LIBRARIES)
 
  if (FREENECT_FOUND)
    if (NOT FREENECT_FIND_QUIETLY)
      message(STATUS "Found Freenect: ${FREENECT_LIBRARIES}")
    endif (NOT FREENECT_FIND_QUIETLY)
  else (FREENECT_FOUND)
    if (FREENECT_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find Freenect")
    endif (FREENECT_FIND_REQUIRED)
  endif (FREENECT_FOUND)

  # show the FREENECT_INCLUDE_DIRS and FREENECT_LIBRARIES variables only in the advanced view
  mark_as_advanced(FREENECT_INCLUDE_DIRS FREENECT_LIBRARIES)

endif (FREENECT_LIBRARIES AND FREENECT_INCLUDE_DIRS)
