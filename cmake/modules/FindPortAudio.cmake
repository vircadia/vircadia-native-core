#  Find the static PortAudio library
#
#  You must provide a PORTAUDIO_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  PORTAUDIO_FOUND - system found PortAudio
#  PORTAUDIO_INCLUDE_DIRS - the PortAudio include directory
#  PORTAUDIO_LIBRARY - Link this to use PortAudio
#
#  Created on 5/14/2013 by Stephen Birarda
#  Copyright (c) 2013 High Fidelity
#

if (PORTAUDIO_LIBRARY AND PORTAUDIO_INCLUDE_DIRS)
  # in cache already
  set(PORTAUDIO_FOUND TRUE)
else (PORTAUDIO_LIBRARY AND PORTAUDIO_INCLUDE_DIRS)
  set(PORTAUDIO_INCLUDE_DIRS ${PORTAUDIO_ROOT_DIR}/include)

  if (APPLE)
      set(PORTAUDIO_LIBRARY ${PORTAUDIO_ROOT_DIR}/lib/MacOS/libportaudio.a)
  else (WIN32)
      set(PORTAUDIO_LIBRARY ${PORTAUDIO_ROOT_DIR}/lib/UNIX/libportaudio.a)
  endif ()

  if (PORTAUDIO_INCLUDE_DIRS AND PORTAUDIO_LIBRARY)
     set(PORTAUDIO_FOUND TRUE)
  endif (PORTAUDIO_INCLUDE_DIRS AND PORTAUDIO_LIBRARY)
 
  if (PORTAUDIO_FOUND)
    if (NOT PortAudio_FIND_QUIETLY)
      message(STATUS "Found PortAudio: ${PORTAUDIO_LIBRARY}")
    endif (NOT PortAudio_FIND_QUIETLY)
  else (PORTAUDIO_FOUND)
    if (PortAudio_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find PortAudio")
    endif (PortAudio_FIND_REQUIRED)
  endif (PORTAUDIO_FOUND)

  # show the PORTAUDIO_INCLUDE_DIRS and PORTAUDIO_LIBRARY variables only in the advanced view
  mark_as_advanced(PORTAUDIO_INCLUDE_DIRS PORTAUDIO_LIBRARY)

endif (PORTAUDIO_LIBRARY AND PORTAUDIO_INCLUDE_DIRS)