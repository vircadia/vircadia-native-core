#  Find the static Quazip library
#
#  You must provide a QUAZIP_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  QUAZIP_FOUND - system found quazip
#  QUAZIP_INCLUDE_DIRS - the quazip include directory
#  QUAZIP_LIBRARIES - Link this to use quazip
#
#  Created on 6/25/2013 by Stephen Birarda
#  Copyright (c) 2013 High Fidelity
#

if (QUAZIP_LIBRARIES AND QUAZIP_INCLUDE_DIRS)
  # in cache already
  set(QUAZIP_FOUND TRUE)
else (QUAZIP_LIBRARIES AND QUAZIP_INCLUDE_DIRS)
  find_path(QUAZIP_INCLUDE_DIRS quazip.h ${QUAZIP_ROOT_DIR}/include)

  if (APPLE)
    find_library(QUAZIP_LIBRARIES libquazip.a ${QUAZIP_ROOT_DIR}/lib/MacOS/)
  endif ()

  if (QUAZIP_INCLUDE_DIRS AND QUAZIP_LIBRARIES)
    set(QUAZIP_FOUND TRUE)
  endif (QUAZIP_INCLUDE_DIRS AND QUAZIP_LIBRARIES)
 
  if (QUAZIP_FOUND)
    if (NOT QUAZIP_FIND_QUIETLY)
      message(STATUS "Found quazip: ${QUAZIP_LIBRARIES}")
    endif (NOT QUAZIP_FIND_QUIETLY)
  else (QUAZIP_FOUND)
    if (QUAZIP_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find quazip")
    endif (QUAZIP_FIND_REQUIRED)
  endif (QUAZIP_FOUND)

  # show the QUAZIP_INCLUDE_DIRS and QUAZIP_LIBRARIES variables only in the advanced view
  mark_as_advanced(QUAZIP_INCLUDE_DIRS QUAZIP_LIBRARIES)

endif (QUAZIP_LIBRARIES AND QUAZIP_INCLUDE_DIRS)