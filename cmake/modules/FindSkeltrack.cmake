#  Try to find the Skeltrack library to perform skeleton tracking via depth camera
#
#  You must provide a SKELTRACK_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  SKELTRACK_FOUND - system found Skeltrack
#  SKELTRACK_INCLUDE_DIRS - the Skeltrack include directory
#  SKELTRACK_LIBRARIES - Link this to use Skeltrack
#
#  Created on 6/25/2013 by Andrzej Kapolka
#  Copyright (c) 2013 High Fidelity
#

if (SKELTRACK_LIBRARIES AND SKELTRACK_INCLUDE_DIRS)
  # in cache already
  set(SKELTRACK_FOUND TRUE)
else (SKELTRACK_LIBRARIES AND SKELTRACK_INCLUDE_DIRS)
  find_path(SKELTRACK_INCLUDE_DIRS skeltrack.h ${SKELTRACK_ROOT_DIR}/include)
  
  if (APPLE)
    find_library(SKELTRACK_LIBRARIES libskeltrack.a ${SKELTRACK_ROOT_DIR}/lib/MacOS/)
  elseif (UNIX)
    find_library(SKELTRACK_LIBRARIES libskeltrack.a ${SKELTRACK_ROOT_DIR}/lib/UNIX/)
  endif ()
  
  if (SKELTRACK_INCLUDE_DIRS AND SKELTRACK_LIBRARIES)
     set(SKELTRACK_FOUND TRUE)
  endif (SKELTRACK_INCLUDE_DIRS AND SKELTRACK_LIBRARIES)
 
  if (SKELTRACK_FOUND)
    if (NOT SKELTRACK_FIND_QUIETLY)
      message(STATUS "Found Skeltrack: ${SKELTRACK_LIBRARIES}")
    endif (NOT SKELTRACK_FIND_QUIETLY)
  else (SKELTRACK_FOUND)
    if (SKELTRACK_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find Skeltrack")
    endif (SKELTRACK_FIND_REQUIRED)
  endif (SKELTRACK_FOUND)

  # show the SKELTRACK_INCLUDE_DIRS and SKELTRACK_LIBRARIES variables only in the advanced view
  mark_as_advanced(SKELTRACK_INCLUDE_DIRS SKELTRACK_LIBRARIES)

endif (SKELTRACK_LIBRARIES AND SKELTRACK_INCLUDE_DIRS)
