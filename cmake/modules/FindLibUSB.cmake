#  Try to find the LibUSB library to interact with USB devices
#
#  You must provide a LIBUSB_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  LIBUSB_FOUND - system found LibUSB
#  LIBUSB_INCLUDE_DIRS - the LibUSB include directory
#  LIBUSB_LIBRARIES - Link this to use LibUSB
#
#  Created on 6/25/2013 by Andrzej Kapolka
#  Copyright (c) 2013 High Fidelity
#

if (LIBUSB_LIBRARIES AND LIBUSB_INCLUDE_DIRS)
  # in cache already
  set(LIBUSB_FOUND TRUE)
else (LIBUSB_LIBRARIES AND LIBUSB_INCLUDE_DIRS)
  find_path(LIBUSB_INCLUDE_DIRS libusb.h ${LIBUSB_ROOT_DIR}/include)
  
  if (APPLE)
    find_library(LIBUSB_LIBRARIES libusb-1.0.a ${LIBUSB_ROOT_DIR}/lib/MacOS/)
  elseif (UNIX)
    find_library(LIBUSB_LIBRARIES libusb-1.0.a ${LIBUSB_ROOT_DIR}/lib/UNIX/)
  endif ()
  
  if (LIBUSB_INCLUDE_DIRS AND LIBUSB_LIBRARIES)
     set(LIBUSB_FOUND TRUE)
  endif (LIBUSB_INCLUDE_DIRS AND LIBUSB_LIBRARIES)
 
  if (LIBUSB_FOUND)
    if (NOT LIBUSB_FIND_QUIETLY)
      message(STATUS "Found LibUSB: ${LIBUSB_LIBRARIES}")
    endif (NOT LIBUSB_FIND_QUIETLY)
  else (LIBUSB_FOUND)
    if (LIBUSB_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find LibUSB")
    endif (LIBUSB_FIND_REQUIRED)
  endif (LIBUSB_FOUND)

  # show the LIBUSB_INCLUDE_DIRS and LIBUSB_LIBRARIES variables only in the advanced view
  mark_as_advanced(LIBUSB_INCLUDE_DIRS LIBUSB_LIBRARIES)

endif (LIBUSB_LIBRARIES AND LIBUSB_INCLUDE_DIRS)
