#  Find the NITE library
#
#  You must provide an NITE_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  NITE_FOUND - system found NITE
#  NITE_INCLUDE_DIRS - the NITE include directory
#  NITE_LIBRARIES - Link this to use NITE
#
#  Created on 6/28/2013 by Andrzej Kapolka
#  Copyright (c) 2013 High Fidelity
#

if (NITE_LIBRARIES AND NITE_INCLUDE_DIRS)
  # in cache already
  set(NITE_FOUND TRUE)
else (NITE_LIBRARIES AND NITE_INCLUDE_DIRS)
  find_path(NITE_INCLUDE_DIRS XnVNite.h /usr/include/nite)
  
  if (APPLE)
    find_library(NITE_LIBRARIES libXnVNite_1_5_2.dylib /usr/lib)
  elseif (UNIX)
    find_library(NITE_LIBRARIES libXnVNite_1_5_2.so /usr/lib)
  endif ()

  if (NITE_INCLUDE_DIRS AND NITE_LIBRARIES)
    set(NITE_FOUND TRUE)
  endif (NITE_INCLUDE_DIRS AND NITE_LIBRARIES)
 
  if (NITE_FOUND)
    if (NOT NITE_FIND_QUIETLY)
      message(STATUS "Found NITE: ${NITE_LIBRARIES}")
    endif (NOT NITE_FIND_QUIETLY)
  else (NITE_FOUND)
    if (NITE_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find NITE")
    endif (NITE_FIND_REQUIRED)
  endif (NITE_FOUND)

  # show the NITE_INCLUDE_DIRS and NITE_LIBRARIES variables only in the advanced view
  mark_as_advanced(NITE_INCLUDE_DIRS NITE_LIBRARIES)

endif (NITE_LIBRARIES AND NITE_INCLUDE_DIRS)
