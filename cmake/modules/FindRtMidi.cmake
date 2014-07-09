#
#  FindRtMidd.cmake
# 
#  Try to find the RtMidi library
#
#  You can provide a RTMIDI_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  RTMIDI_FOUND - system found RtMidi
#  RTMIDI_INCLUDE_DIRS - the RtMidi include directory
#  RTMIDI_CPP - Include this with src to use RtMidi
#
#  Created on 6/30/2014 by Stephen Birarda
#  Copyright 2014 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

if (RTMIDI_LIBRARIES AND RTMIDI_INCLUDE_DIRS)
  # in cache already
  set(RTMIDI_FOUND TRUE)
else ()
  
  set(RTMIDI_SEARCH_DIRS "${RTMIDI_ROOT_DIR}" "$ENV{HIFI_LIB_DIR}/rtmidi")
  
  find_path(RTMIDI_INCLUDE_DIR RtMidi.h PATH_SUFFIXES include HINTS ${RTMIDI_SEARCH_DIRS})
  find_library(RTMIDI_LIBRARY NAMES rtmidi PATH_SUFFIXES lib HINTS ${RTMIDI_SEARCH_DIRS})
  
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(RTMIDI DEFAULT_MSG RTMIDI_INCLUDE_DIR RTMIDI_LIBRARY)
endif ()