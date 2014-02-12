#  Try to find the Visage controller library
#
#  You must provide a VISAGE_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  VISAGE_FOUND - system found Visage
#  VISAGE_INCLUDE_DIRS - the Visage include directory
#  VISAGE_LIBRARIES - Link this to use Visage
#
#  Created on 2/11/2014 by Andrzej Kapolka
#  Copyright (c) 2014 High Fidelity
#

if (VISAGE_LIBRARIES AND VISAGE_INCLUDE_DIRS)
  # in cache already
  set(VISAGE_FOUND TRUE)
else (VISAGE_LIBRARIES AND VISAGE_INCLUDE_DIRS)
  find_path(VISAGE_INCLUDE_DIRS VisageTracker2.h ${VISAGE_ROOT_DIR}/include)

  if (APPLE)
    find_library(VISAGE_LIBRARIES libvscore.a ${VISAGE_ROOT_DIR}/lib)
  elseif (WIN32)
    find_library(VISAGE_LIBRARIES vscore.dll ${VISAGE_ROOT_DIR}/lib)
  endif ()

  if (VISAGE_INCLUDE_DIRS AND VISAGE_LIBRARIES)
     set(VISAGE_FOUND TRUE)
  endif (VISAGE_INCLUDE_DIRS AND VISAGE_LIBRARIES)
 
  if (VISAGE_FOUND)
    if (NOT VISAGE_FIND_QUIETLY)
      message(STATUS "Found Visage: ${VISAGE_LIBRARIES}")
    endif (NOT VISAGE_FIND_QUIETLY)
  else (VISAGE_FOUND)
    if (VISAGE_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find Visage")
    endif (VISAGE_FIND_REQUIRED)
  endif (VISAGE_FOUND)

  # show the VISAGE_INCLUDE_DIRS and VISAGE_LIBRARIES variables only in the advanced view
  mark_as_advanced(VISAGE_INCLUDE_DIRS VISAGE_LIBRARIES)

endif (VISAGE_LIBRARIES AND VISAGE_INCLUDE_DIRS)
