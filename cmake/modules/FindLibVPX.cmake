# - Try to find the LibVPX library to perform VP8 video encoding/decoding
#
#  You must provide a LIBVPX_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  LIBVPX_FOUND - system found LibVPX
#  LIBVPX_INCLUDE_DIRS - the LibVPX include directory
#  LIBVPX_LIBRARIES - Link this to use LibVPX
#
#  Created on 7/17/2013 by Andrzej Kapolka
#  Copyright (c) 2013 High Fidelity
#

if (LIBVPX_LIBRARIES AND LIBVPX_INCLUDE_DIRS)
  # in cache already
  set(LIBVPX_FOUND TRUE)
else (LIBVPX_LIBRARIES AND LIBVPX_INCLUDE_DIRS)
  find_path(LIBVPX_INCLUDE_DIRS vpx_codec.h ${LIBVPX_ROOT_DIR}/include)

  if (APPLE)
    find_library(LIBVPX_LIBRARIES libvpx.a ${LIBVPX_ROOT_DIR}/lib/MacOS/)
  elseif (UNIX)
    find_library(LIBVPX_LIBRARIES libvpx.a ${LIBVPX_ROOT_DIR}/lib/UNIX/)
  elseif (WIN32)
    find_library(LIBVPX_LIBRARIES libvpx.lib ${LIBVPX_ROOT_DIR}/lib/Win32/)
  endif ()

  if (LIBVPX_INCLUDE_DIRS AND LIBVPX_LIBRARIES)
     set(LIBVPX_FOUND TRUE)
  endif (LIBVPX_INCLUDE_DIRS AND LIBVPX_LIBRARIES)
 
  if (LIBVPX_FOUND)
    if (NOT LibVPX_FIND_QUIETLY)
      message(STATUS "Found LibVPX: ${LIBVPX_LIBRARIES}")
    endif (NOT LibVPX_FIND_QUIETLY)
  else (LIBVPX_FOUND)
    if (LibVPX_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find LibVPX")
    endif (LibVPX_FIND_REQUIRED)
  endif (LIBVPX_FOUND)

  # show the LIBVPX_INCLUDE_DIRS and LIBVPX_LIBRARIES variables only in the advanced view
  mark_as_advanced(LIBVPX_INCLUDE_DIRS LIBVPX_LIBRARIES)

endif (LIBVPX_LIBRARIES AND LIBVPX_INCLUDE_DIRS)
