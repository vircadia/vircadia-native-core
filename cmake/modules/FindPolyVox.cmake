#
#  FindPolyvox.cmake
# 
#  Try to find the libpolyvox resampling library
#
#  You can provide a LIBPOLYVOX_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  POLYVOX_FOUND - system found libpolyvox
#  POLYVOX_INCLUDE_DIRS - the libpolyvox include directory
#  POLYVOX_LIBRARIES - link to this to use libpolyvox
#
#  Created on 1/22/2015 by Stephen Birarda
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("polyvox")

find_path(POLYVOX_CORE_INCLUDE_DIRS PolyVoxCore/SimpleVolume.h PATH_SUFFIXES include include/PolyVoxCore HINTS ${POLYVOX_SEARCH_DIRS})
# find_path(POLYVOX_UTIL_INCLUDE_DIRS PolyVoxUtil/Serialization.h PATH_SUFFIXES include include/PolyVoxUtil HINTS ${POLYVOX_SEARCH_DIRS})

find_library(POLYVOX_CORE_LIBRARY_DEBUG NAMES PolyVoxCore PATH_SUFFIXES lib/Debug HINTS ${POLYVOX_SEARCH_DIRS})
find_library(POLYVOX_CORE_LIBRARY_RELEASE NAMES PolyVoxCore PATH_SUFFIXES lib/Release lib HINTS ${POLYVOX_SEARCH_DIRS})
# find_library(POLYVOX_UTIL_LIBRARY NAMES PolyVoxUtil PATH_SUFFIXES lib HINTS ${POLYVOX_SEARCH_DIRS})

include(SelectLibraryConfigurations)
select_library_configurations(POLYVOX_CORE)

# if (WIN32)
#   find_path(POLYVOX_DLL_PATH polyvox.dll PATH_SUFFIXES bin HINTS ${POLYVOX_SEARCH_DIRS}) 
# endif()

# set(POLYVOX_REQUIREMENTS POLYVOX_CORE_INCLUDE_DIRS POLYVOX_UTIL_INCLUDE_DIRS POLYVOX_CORE_LIBRARY POLYVOX_UTIL_LIBRARY)
set(POLYVOX_REQUIREMENTS POLYVOX_CORE_INCLUDE_DIRS POLYVOX_CORE_LIBRARY)
# if (WIN32)
#   list(APPEND POLYVOX_REQUIREMENTS POLYVOX_DLL_PATH)
# endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Polyvox DEFAULT_MSG ${POLYVOX_REQUIREMENTS})

# if (WIN32)
#   add_paths_to_fixup_libs(${POLYVOX_DLL_PATH})
# endif ()

# set(POLYVOX_INCLUDE_DIRS ${POLYVOX_CORE_INCLUDE_DIRS} ${POLYVOX_UTIL_INCLUDE_DIRS})
# set(POLYVOX_LIBRARIES ${POLYVOX_CORE_LIBRARY} ${POLYVOX_UTIL_LIBRARY})

set(POLYVOX_INCLUDE_DIRS ${POLYVOX_CORE_INCLUDE_DIRS})
set(POLYVOX_LIBRARIES ${POLYVOX_CORE_LIBRARY})

mark_as_advanced(POLYVOX_INCLUDE_DIRS POLYVOX_LIBRARIES POLYVOX_SEARCH_DIRS)
