#  Find the OpenNI library
#
#  You must provide an OPENNI_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  OPENNI_FOUND - system found OpenNI
#  OPENNI_INCLUDE_DIRS - the OpenNI include directory
#  OPENNI_LIBRARIES - Link this to use OpenNI
#
#  Created on 6/28/2013 by Andrzej Kapolka
#  Copyright (c) 2013 High Fidelity
#

if (OPENNI_LIBRARIES AND OPENNI_INCLUDE_DIRS)
  # in cache already
  set(OPENNI_FOUND TRUE)
else (OPENNI_LIBRARIES AND OPENNI_INCLUDE_DIRS)
  find_path(OPENNI_INCLUDE_DIRS XnOpenNI.h /usr/include/ni)
  
  if (APPLE)
    find_library(OPENNI_LIBRARIES libOpenNI.dylib /usr/lib)
  elseif (UNIX)
    find_library(OPENNI_LIBRARIES libOpenNI.so /usr/lib)
  endif ()

  if (OPENNI_INCLUDE_DIRS AND OPENNI_LIBRARIES)
    set(OPENNI_FOUND TRUE)
  endif (OPENNI_INCLUDE_DIRS AND OPENNI_LIBRARIES)
 
  if (OPENNI_FOUND)
    if (NOT OPENNI_FIND_QUIETLY)
      message(STATUS "Found OpenNI: ${OPENNI_LIBRARIES}")
    endif (NOT OPENNI_FIND_QUIETLY)
  else (OPENNI_FOUND)
    if (OPENNI_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find OpenNI")
    endif (OPENNI_FIND_REQUIRED)
  endif (OPENNI_FOUND)

  # show the OPENNI_INCLUDE_DIRS and OPENNI_LIBRARIES variables only in the advanced view
  mark_as_advanced(OPENNI_INCLUDE_DIRS OPENNI_LIBRARIES)

endif (OPENNI_LIBRARIES AND OPENNI_INCLUDE_DIRS)
