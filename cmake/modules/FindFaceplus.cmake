#  Try to find the Faceplus library
#
#  You must provide a FACEPLUS_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  FACEPLUS_FOUND - system found Faceplus
#  FACEPLUS_INCLUDE_DIRS - the Faceplus include directory
#  FACEPLUS_LIBRARIES - Link this to use Faceplus
#
#  Created on 4/8/2014 by Andrzej Kapolka
#  Copyright (c) 2014 High Fidelity
#

if (FACEPLUS_LIBRARIES AND FACEPLUS_INCLUDE_DIRS)
  # in cache already
  set(FACEPLUS_FOUND TRUE)
else (FACEPLUS_LIBRARIES AND FACEPLUS_INCLUDE_DIRS)
  find_path(FACEPLUS_INCLUDE_DIRS faceplus.h ${FACEPLUS_ROOT_DIR}/include)

  if (WIN32)
    find_library(FACEPLUS_LIBRARIES faceplus.lib ${FACEPLUS_ROOT_DIR}/win32/)
  endif (WIN32)
 
  if (FACEPLUS_INCLUDE_DIRS AND FACEPLUS_LIBRARIES)
    set(FACEPLUS_FOUND TRUE)
  endif (FACEPLUS_INCLUDE_DIRS AND FACEPLUS_LIBRARIES)

  if (FACEPLUS_FOUND)
    if (NOT FACEPLUS_FIND_QUIETLY)
      message(STATUS "Found Faceplus... ${FACEPLUS_LIBRARIES}")
    endif (NOT FACEPLUS_FIND_QUIETLY)
  else ()
    if (FACEPLUS_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find Faceplus")
    endif (FACEPLUS_FIND_REQUIRED)
  endif ()

  # show the FACEPLUS_INCLUDE_DIRS and FACEPLUS_LIBRARIES variables only in the advanced view
  mark_as_advanced(FACEPLUS_INCLUDE_DIRS FACEPLUS_LIBRARIES)

endif (FACEPLUS_LIBRARIES AND FACEPLUS_INCLUDE_DIRS)
