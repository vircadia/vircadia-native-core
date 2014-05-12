#  Try to find the PrioVT library
#
#  You must provide a PRIOVR_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  PRIOVR_FOUND - system found PrioVR
#  PRIOVR_INCLUDE_DIRS - the PrioVR include directory
#  PRIOVR_LIBRARIES - Link this to use PrioVR
#
#  Created on 5/12/2014 by Andrzej Kapolka
#  Copyright (c) 2014 High Fidelity
#

if (PRIOVR_LIBRARIES AND PRIOVR_INCLUDE_DIRS)
  # in cache already
  set(PRIOVR_FOUND TRUE)
else (PRIOVR_LIBRARIES AND PRIOVR_INCLUDE_DIRS)
  find_path(PRIOVR_INCLUDE_DIRS yei_threespace_api.h ${PRIOVR_ROOT_DIR}/include)

  if (WIN32)
    find_library(PRIOVR_LIBRARIES ThreeSpace_API.lib ${PRIOVR_ROOT_DIR})
  endif (WIN32)
 
  if (PRIOVR_INCLUDE_DIRS AND PRIOVR_LIBRARIES)
    set(PRIOVR_FOUND TRUE)
  endif (PRIOVR_INCLUDE_DIRS AND PRIOVR_LIBRARIES)

  if (PRIOVR_FOUND)
    if (NOT PRIOVR_FIND_QUIETLY)
      message(STATUS "Found PrioVR... ${PRIOVR_LIBRARIES}")
    endif (NOT PRIOVR_FIND_QUIETLY)
  else ()
    if (PRIOVR_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find PrioVR")
    endif (PRIOVR_FIND_REQUIRED)
  endif ()

  # show the PRIOVR_INCLUDE_DIRS and PRIOVR_LIBRARIES variables only in the advanced view
  mark_as_advanced(PRIOVR_INCLUDE_DIRS PRIOVR_LIBRARIES)

endif (PRIOVR_LIBRARIES AND PRIOVR_INCLUDE_DIRS)
