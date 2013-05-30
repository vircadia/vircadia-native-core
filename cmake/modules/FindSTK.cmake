#  Find the static STK library
#
#  You must provide an STK_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  STK_FOUND - system found PortAudio
#  STK_INCLUDE_DIRS - the PortAudio include directory
#  STK_LIBRARIES - Link this to use PortAudio
#
#  Created on 5/30/2013 by Stephen Birarda
#  Copyright (c) 2013 High Fidelity
#

if (STK_LIBRARIES AND STK_INCLUDE_DIRS)
  # in cache already
  set(STK_FOUND TRUE)
else (STK_LIBRARIES AND STK_INCLUDE_DIRS)
  find_path(STK_INCLUDE_DIRS Stk.h ${STK_ROOT_DIR}/include)

  if (APPLE)
    find_library(STK_LIBRARIES libstk.a ${STK_ROOT_DIR}/lib/MacOS/)
  elseif (UNIX)
    find_library(STK_LIBRARIES libstk.a ${STK_ROOT_DIR}/lib/UNIX/)
  endif ()

  if (STK_INCLUDE_DIRS AND STK_LIBRARIES)
    set(STK_FOUND TRUE)
  endif (STK_INCLUDE_DIRS AND STK_LIBRARIES)
 
  if (STK_FOUND)
    if (NOT STK_FIND_QUIETLY)
      message(STATUS "Found STK: ${STK_LIBRARIES}")
    endif (NOT STK_FIND_QUIETLY)
  else (STK_FOUND)
    if (STK_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find STK")
    endif (STK_FIND_REQUIRED)
  endif (STK_FOUND)

  # show the STK_INCLUDE_DIRS and STK_LIBRARIES variables only in the advanced view
  mark_as_advanced(STK_INCLUDE_DIRS STK_LIBRARIES)

endif (STK_LIBRARIES AND STK_INCLUDE_DIRS)