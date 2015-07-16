# 
#  FindSixense.cmake 
# 
#  Try to find the Sixense controller library
#
#  You must provide a SIXENSE_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  SIXENSE_FOUND - system found Sixense
#  SIXENSE_INCLUDE_DIRS - the Sixense include directory
#  SIXENSE_LIBRARIES - Link this to use Sixense
#
#  Created on 11/15/2013 by Andrzej Kapolka
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("sixense")

find_path(SIXENSE_INCLUDE_DIRS sixense.h PATH_SUFFIXES include HINTS ${SIXENSE_SEARCH_DIRS})

if (APPLE)
  find_library(SIXENSE_LIBRARY_RELEASE lib/osx_x64/release_dll/libsixense_x64.dylib HINTS ${SIXENSE_SEARCH_DIRS})
  find_library(SIXENSE_LIBRARY_DEBUG lib/osx_x64/debug_dll/libsixensed_x64.dylib HINTS ${SIXENSE_SEARCH_DIRS})
elseif (UNIX)
  find_library(SIXENSE_LIBRARY_RELEASE lib/linux_x64/release/libsixense_x64.so HINTS ${SIXENSE_SEARCH_DIRS})
  # find_library(SIXENSE_LIBRARY_DEBUG lib/linux_x64/debug/libsixensed_x64.so HINTS ${SIXENSE_SEARCH_DIRS})
elseif (WIN32)
    if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        set(ARCH_DIR "x64")
        set(ARCH_SUFFIX "_x64")
    else()
        set(ARCH_DIR "Win32")
        set(ARCH_SUFFIX "")
    endif()

  find_library(SIXENSE_LIBRARY_RELEASE "lib/${ARCH_DIR}/release_dll/sixense${ARCH_SUFFIX}.lib" HINTS ${SIXENSE_SEARCH_DIRS})
  find_library(SIXENSE_LIBRARY_DEBUG "lib/${ARCH_DIR}/debug_dll/sixensed.lib" HINTS ${SIXENSE_SEARCH_DIRS})
  
  find_path(SIXENSE_DEBUG_DLL_PATH "sixensed${ARCH_SUFFIX}.dll" PATH_SUFFIXES bin/${ARCH_DIR}/debug_dll HINTS ${SIXENSE_SEARCH_DIRS})
  find_path(SIXENSE_RELEASE_DLL_PATH "sixense${ARCH_SUFFIX}.dll" PATH_SUFFIXES bin/${ARCH_DIR}/release_dll HINTS ${SIXENSE_SEARCH_DIRS})
  find_path(SIXENSE_DEVICE_DLL_PATH DeviceDLL.dll PATH_SUFFIXES samples/${ARCH_DIR}/sixense_simple3d HINTS ${SIXENSE_SEARCH_DIRS})
endif ()

include(SelectLibraryConfigurations)
select_library_configurations(SIXENSE)

set(SIXENSE_REQUIREMENTS SIXENSE_INCLUDE_DIRS SIXENSE_LIBRARIES)
if (WIN32)
  list(APPEND SIXENSE_REQUIREMENTS SIXENSE_DEBUG_DLL_PATH SIXENSE_RELEASE_DLL_PATH SIXENSE_DEVICE_DLL_PATH)
endif ()

set(SIXENSE_LIBRARIES "${SIXENSE_LIBRARY}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sixense DEFAULT_MSG ${SIXENSE_REQUIREMENTS})

if (WIN32)
  add_paths_to_fixup_libs(${SIXENSE_DEBUG_DLL_PATH} ${SIXENSE_RELEASE_DLL_PATH} ${SIXENSE_DEVICE_DLL_PATH})
endif ()

mark_as_advanced(SIXENSE_LIBRARIES SIXENSE_INCLUDE_DIRS SIXENSE_SEARCH_DIRS)
