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
else ()
  
  set(SIXENSE_SEARCH_DIRS "${SIXENSE_ROOT_DIR}" "$ENV{HIFI_LIB_DIR}/sixense")
  
  find_path(SIXENSE_INCLUDE_DIRS include/sixense.h HINTS ${SIXENSE_SEARCH_DIRS})

  if (APPLE)
    find_library(SIXENSE_LIBRARIES lib/osx_x64/release_dll/libsixense_x64.dylib HINTS ${SIXENSE_SEARCH_DIRS})
  elseif (UNIX)
    find_library(SIXENSE_LIBRARIES lib/linux_x64/release/libsixense_x64.so HINTS ${SIXENSE_SEARCH_DIRS})
  endif ()

  if (SIXENSE_INCLUDE_DIRS AND SIXENSE_LIBRARIES)
     set(SIXENSE_FOUND TRUE)
  endif ()
 
  if (SIXENSE_FOUND)
    if (NOT SIXENSE_FIND_QUIETLY)
      message(STATUS "Found Sixense: ${SIXENSE_LIBRARIES}")
    endif (NOT SIXENSE_FIND_QUIETLY)
  else ()
    if (SIXENSE_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find Sixense")
    endif (SIXENSE_FIND_REQUIRED)
  endif ()
endif ()
