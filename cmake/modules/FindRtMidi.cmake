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
#  RTMIDI_LIBRARIES - link to this to use RtMidi
#
#  Created on 6/30/2014 by Stephen Birarda
#  Copyright 2014 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("rtmidi")

find_path(RTMIDI_INCLUDE_DIRS RtMidi.h PATH_SUFFIXES include HINTS ${RTMIDI_SEARCH_DIRS})
find_library(RTMIDI_LIBRARIES NAMES rtmidi PATH_SUFFIXES lib HINTS ${RTMIDI_SEARCH_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RtMidi DEFAULT_MSG RTMIDI_INCLUDE_DIRS RTMIDI_LIBRARIES)

mark_as_advanced(RTMIDI_INCLUDE_DIRS RTMIDI_LIBRARIES RTMIDI_SEARCH_DIRS)