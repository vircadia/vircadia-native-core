#  Try to find the MotionDriver library to access the InvenSense gyros
#
#  You must provide a MOTIONDRIVER_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  MOTIONDRIVER_FOUND - system found MotionDriver
#  MOTIONDRIVER_INCLUDE_DIRS - the MotionDriver include directory
#  MOTIONDRIVER_LIBRARIES - Link this to use MotionDriver
#
#  Created on 7/9/2013 by Andrzej Kapolka
#  Copyright (c) 2013 High Fidelity
#

if (MOTIONDRIVER_LIBRARIES AND MOTIONDRIVER_INCLUDE_DIRS)
  # in cache already
  set(MOTIONDRIVER_FOUND TRUE)
else (MOTIONDRIVER_LIBRARIES AND MOTIONDRIVER_INCLUDE_DIRS)
  find_path(MOTIONDRIVER_INCLUDE_DIRS inv_mpu.h ${MOTIONDRIVER_ROOT_DIR}/include)

  if (APPLE)
    find_library(MOTIONDRIVER_LIBRARIES libMotionDriver.a ${MOTIONDRIVER_ROOT_DIR}/lib/MacOS/)
  elseif (UNIX)
    find_library(MOTIONDRIVER_LIBRARIES libMotionDriver.a ${MOTIONDRIVER_ROOT_DIR}/lib/UNIX/)
  endif ()

  if (MOTIONDRIVER_INCLUDE_DIRS AND MOTIONDRIVER_LIBRARIES)
     set(MOTIONDRIVER_FOUND TRUE)
  endif (MOTIONDRIVER_INCLUDE_DIRS AND MOTIONDRIVER_LIBRARIES)
 
  if (MOTIONDRIVER_FOUND)
    if (NOT MOTIONDRIVER_FIND_QUIETLY)
      message(STATUS "Found MotionDriver: ${MOTIONDRIVER_LIBRARIES}")
    endif (NOT MOTIONDRIVER_FIND_QUIETLY)
  else (MOTIONDRIVER_FOUND)
    if (MOTIONDRIVER_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find MotionDriver")
    endif (MOTIONDRIVER_FIND_REQUIRED)
  endif (MOTIONDRIVER_FOUND)

  # show the MOTIONDRIVER_INCLUDE_DIRS and MOTIONDRIVER_LIBRARIES variables only in the advanced view
  mark_as_advanced(MOTIONDRIVER_INCLUDE_DIRS MOTIONDRIVER_LIBRARIES)

endif (MOTIONDRIVER_LIBRARIES AND MOTIONDRIVER_INCLUDE_DIRS)
