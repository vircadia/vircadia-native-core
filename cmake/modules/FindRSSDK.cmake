#  Try to find the RSSDK library
#
#  You must provide a RSSDK_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  RSSDK_FOUND - system found RSSDK
#  RSSDK_INCLUDE_DIRS - the RSSDK include directory
#  RSSDK_LIBRARIES - Link this to use RSSDK
#
#  Created on 12/7/2014 by Thijs Wenker
#  Copyright (c) 2014 High Fidelity
#

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("rssdk")

find_path(RSSDK_INCLUDE_DIRS pxcbase.h PATH_SUFFIXES include HINTS ${RSSDK_SEARCH_DIRS})

if (WIN32)
  find_library(RSSDK_LIBRARY_DEBUG libpxc_d PATH_SUFFIXES lib/Win32 HINTS ${RSSDK_SEARCH_DIRS})
  find_library(RSSDK_LIBRARY_RELEASE libpxc PATH_SUFFIXES lib/Win32 HINTS ${RSSDK_SEARCH_DIRS})
endif ()

include(SelectLibraryConfigurations)
select_library_configurations(RSSDK)

set(RSSDK_LIBRARIES "${RSSDK_LIBRARY}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RSSDK DEFAULT_MSG RSSDK_INCLUDE_DIRS RSSDK_LIBRARIES)

mark_as_advanced(RSSDK_INCLUDE_DIRS RSSDK_LIBRARIES RSSDK_SEARCH_DIRS)
