#  Try to find the LeapMotion library
#
#  You must provide a LEAPMOTION_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  LEAPMOTION_FOUND - system found LEAPMOTION
#  LEAPMOTION_INCLUDE_DIRS - the LEAPMOTION include directory
#  LEAPMOTION_LIBRARIES - Link this to use LEAPMOTION
#
#  Created on 6/2/2014 by Sam Cake
#  Copyright (c) 2014 High Fidelity
#

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("leapmotion")

find_path(LEAPMOTION_INCLUDE_DIRS Leap.h PATH_SUFFIXES include HINTS ${LEAPMOTION_SEARCH_DIRS})

if (WIN32)
  find_library(LEAPMOTION_LIBRARY_DEBUG Leapd PATH_SUFFIXES lib/x86 HINTS ${LEAPMOTION_SEARCH_DIRS})
  find_library(LEAPMOTION_LIBRARY_RELEASE Leap PATH_SUFFIXES lib/x86 HINTS ${LEAPMOTION_SEARCH_DIRS})
elseif (APPLE)
  find_library(LEAPMOTION_LIBRARY_RELEASE Leap PATH_SUFFIXES lib/libc++ lib HINTS ${LEAPMOTION_SEARCH_DIRS})
endif ()

include(SelectLibraryConfigurations)
select_library_configurations(LEAPMOTION)

set(LEAPMOTION_LIBRARIES "${LEAPMOTION_LIBRARY}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LEAPMOTION DEFAULT_MSG LEAPMOTION_INCLUDE_DIRS LEAPMOTION_LIBRARIES)

mark_as_advanced(LEAPMOTION_INCLUDE_DIRS LEAPMOTION_LIBRARIES LEAPMOTION_SEARCH_DIRS)
