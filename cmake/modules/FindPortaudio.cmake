# - Try to find the Portaudio library
#
#  You must provide a PORTAUDIO_ROOT_DIR which contains the header and library
#
#  Once done this will define
#
#  PORTAUDIO_FOUND - system has Portaudio
#  PORTAUDIO_INCLUDE_DIRS - the Portaudio include directory
#  PORTAUDIO_LIBRARY - Link these to use Portaudio
#
#  Copyright (c) 2013 Stephen Birarda <birarda@coffeeandpower.com>
#
#  Heavily based on Andreas Schneider's original FindPortaudio.cmake
#  which can be found at http://gnuradio.org/redmine/projects/gnuradio/repository/


if (PORTAUDIO_LIBRARY AND PORTAUDIO_INCLUDE_DIRS)
  # in cache already
  set(PORTAUDIO_FOUND TRUE)
else (PORTAUDIO_LIBRARY AND PORTAUDIO_INCLUDE_DIRS)

  set(PORTAUDIO_INCLUDE_DIRS
    ${PORTAUDIO_ROOT_DIR}/portaudio.h
  )
  set(PORTAUDIO_LIBRARY
    ${PORTAUDIO_ROOT_DIR}/libportaudio.a
  )
 
  if (PORTAUDIO_INCLUDE_DIRS AND PORTAUDIO_LIBRARY)
     set(PORTAUDIO_FOUND TRUE)
  endif (PORTAUDIO_INCLUDE_DIRS AND PORTAUDIO_LIBRARY)
 
  if (PORTAUDIO_FOUND)
    if (NOT Portaudio_FIND_QUIETLY)
      message(STATUS "Found Portaudio: ${PORTAUDIO_LIBRARY}")
    endif (NOT Portaudio_FIND_QUIETLY)
  else (PORTAUDIO_FOUND)
    if (Portaudio_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find Portaudio")
    endif (Portaudio_FIND_REQUIRED)
  endif (PORTAUDIO_FOUND)

  # show the PORTAUDIO_INCLUDE_DIRS and PORTAUDIO_LIBRARY variables only in the advanced view
  mark_as_advanced(PORTAUDIO_INCLUDE_DIRS PORTAUDIO_LIBRARY)

endif (PORTAUDIO_LIBRARY AND PORTAUDIO_INCLUDE_DIRS)
