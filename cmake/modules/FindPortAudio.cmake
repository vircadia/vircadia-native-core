#  Find the static PortAudio library
#
#  You must provide a PORTAUDIO_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  PORTAUDIO_FOUND - system found PortAudio
#  PORTAUDIO_INCLUDE_DIRS - the PortAudio include directory
#  PORTAUDIO_LIBRARIES - Link this to use PortAudio
#
#  Created on 5/14/2013 by Stephen Birarda
#  Copyright (c) 2013 High Fidelity
#

if (PORTAUDIO_LIBRARIES AND PORTAUDIO_INCLUDE_DIRS)
  # in cache already
  set(PORTAUDIO_FOUND TRUE)
else (PORTAUDIO_LIBRARIES AND PORTAUDIO_INCLUDE_DIRS)
  find_path(PORTAUDIO_INCLUDE_DIRS portaudio.h ${PORTAUDIO_ROOT_DIR}/include)

  if (APPLE)
    find_library(PORTAUDIO_LIBRARIES libportaudio.a ${PORTAUDIO_ROOT_DIR}/lib/MacOS/)
  elseif (UNIX)
    find_library(PORTAUDIO_LIBRARIES libportaudio.a ${PORTAUDIO_ROOT_DIR}/lib/UNIX/)
  endif ()

  if (PORTAUDIO_INCLUDE_DIRS AND PORTAUDIO_LIBRARIES)
    set(PORTAUDIO_FOUND TRUE)
  endif (PORTAUDIO_INCLUDE_DIRS AND PORTAUDIO_LIBRARIES)
 
  if (PORTAUDIO_FOUND)
    if (NOT PortAudio_FIND_QUIETLY)
      message(STATUS "Found PortAudio: ${PORTAUDIO_LIBRARIES}")
    endif (NOT PortAudio_FIND_QUIETLY)
  else (PORTAUDIO_FOUND)
    if (PortAudio_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find PortAudio")
    endif (PortAudio_FIND_REQUIRED)
  endif (PORTAUDIO_FOUND)

  # show the PORTAUDIO_INCLUDE_DIRS and PORTAUDIO_LIBRARIES variables only in the advanced view
  mark_as_advanced(PORTAUDIO_INCLUDE_DIRS PORTAUDIO_LIBRARIES)

endif (PORTAUDIO_LIBRARIES AND PORTAUDIO_INCLUDE_DIRS)