#  Try to find the Sixense controller library
#
#  You must provide a SIXENSE_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  SIXENSE_FOUND - system found Sixense
#  SIXENSE_INCLUDE_DIRS - the Sixense include directory
#  SIXENSE_LIBRARIES - Link this to use Sixense
#
#  Created on 11/15/2013 by Andrzej Kapolka
#  Copyright (c) 2013 High Fidelity
#

if (SIXENSE_LIBRARIES AND SIXENSE_INCLUDE_DIRS)
  # in cache already
  set(SIXENSE_FOUND TRUE)
else (SIXENSE_LIBRARIES AND SIXENSE_INCLUDE_DIRS)
  find_path(SIXENSE_INCLUDE_DIRS sixense.h ${SIXENSE_ROOT_DIR}/include)

  if (APPLE)
    find_library(SIXENSE_LIBRARIES libsixense_x64.dylib ${SIXENSE_ROOT_DIR}/lib/osx_x64/release_dll)
  elseif (UNIX)
    find_library(SIXENSE_LIBRARIES libsixense_x64.so ${SIXENSE_ROOT_DIR}/lib/linux_x64/release)
  endif ()

  if (SIXENSE_INCLUDE_DIRS AND SIXENSE_LIBRARIES)
     set(SIXENSE_FOUND TRUE)
  endif (SIXENSE_INCLUDE_DIRS AND SIXENSE_LIBRARIES)
 
  if (SIXENSE_FOUND)
    if (NOT SIXENSE_FIND_QUIETLY)
      message(STATUS "Found Sixense: ${SIXENSE_LIBRARIES}")
    endif (NOT SIXENSE_FIND_QUIETLY)
  else (SIXENSE_FOUND)
    if (SIXENSE_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find Sixense")
    endif (SIXENSE_FIND_REQUIRED)
  endif (SIXENSE_FOUND)

  # show the SIXENSE_INCLUDE_DIRS and SIXENSE_LIBRARIES variables only in the advanced view
  mark_as_advanced(SIXENSE_INCLUDE_DIRS SIXENSE_LIBRARIES)

endif (SIXENSE_LIBRARIES AND SIXENSE_INCLUDE_DIRS)
