#
#  FindVHACD.cmake
# 
#  Try to find the V-HACD library that decomposes a 3D surface into a set of "near" convex parts.
#
#  Once done this will define
#
#  VHACD_FOUND - system found V-HACD 
#  VHACD_INCLUDE_DIRS - the V-HACD include directory
#  VHACD_LIBRARIES - link to this to use V-HACD
#
#  Created on 2/20/2015 by Virendra Singh
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("vhacd")

macro(_FIND_VHACD_LIBRARY _var)
  set(_${_var}_NAMES ${ARGN})
  find_library(${_var}_LIBRARY_RELEASE
     NAMES ${_${_var}_NAMES}
     HINTS
        ${VHACD_SEARCH_DIRS}
		$ENV{VHACD_ROOT_DIR}
        PATH_SUFFIXES lib lib/Release
  )
    
  find_library(${_var}_LIBRARY_DEBUG
     NAMES ${_${_var}_NAMES}
     HINTS
        ${VHACD_SEARCH_DIRS}
		$ENV{VHACD_ROOT_DIR}
        PATH_SUFFIXES lib lib/Debug
  )
  
  select_library_configurations(${_var})
  
  mark_as_advanced(${_var}_LIBRARY)
  mark_as_advanced(${_var}_LIBRARY)
endmacro()
  

find_path(VHACD_INCLUDE_DIRS VHACD.h PATH_SUFFIXES include HINTS ${VHACD_SEARCH_DIRS} $ENV{VHACD_ROOT_DIR})
if(NOT WIN32)
_FIND_VHACD_LIBRARY(VHACD libVHACD.a)
else()
_FIND_VHACD_LIBRARY(VHACD VHACD_LIB)
endif()
set(VHACD_LIBRARIES ${VHACD_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VHACD "Could NOT find VHACD, try to set the path to VHACD root folder in the system variable VHACD_ROOT_DIR or create a directory vhacd in HIFI_LIB_DIR and paste the necessary files there"
 VHACD_INCLUDE_DIRS VHACD_LIBRARIES)

mark_as_advanced(VHACD_INCLUDE_DIRS VHACD_LIBRARIES VHACD_SEARCH_DIRS)
