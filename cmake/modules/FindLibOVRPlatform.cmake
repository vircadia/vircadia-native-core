#
#  FindLibOVRPlatform.cmake
#
#  Try to find the LibOVRPlatform library to use the Oculus Platform SDK
#
#  You must provide a LIBOVRPLATFORM_ROOT_DIR which contains Windows and Include directories
#
#  Once done this will define
#
#  LIBOVRPLATFORM_FOUND - system found Oculus Platform SDK
#  LIBOVRPLATFORM_INCLUDE_DIRS - the Oculus Platform include directory
#  LIBOVRPLATFORM_LIBRARIES - Link this to use Oculus Platform
#
#  Created on December 16, 2016 by Stephen Birarda
#  Copyright 2016 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#


if (WIN32)
  # setup hints for LIBOVRPLATFORM search
  include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
  hifi_library_search_hints("LibOVRPlatform")

  find_path(LIBOVRPLATFORM_INCLUDE_DIRS OVR_Platform.h PATH_SUFFIXES Include HINTS ${LIBOVRPLATFORM_SEARCH_DIRS})

  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    set(_LIB_NAME LibOVRPlatform64_1.lib)
  else()
    set(_LIB_NAME LibOVRPlatform32_1.lib)
  endif()

  find_library(LIBOVRPLATFORM_LIBRARY_RELEASE NAMES ${_LIB_NAME} PATH_SUFFIXES Windows HINTS ${LIBOVRPLATFORM_SEARCH_DIRS})

  include(SelectLibraryConfigurations)
  select_library_configurations(LIBOVRPLATFORM)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LIBOVRPLATFORM DEFAULT_MSG LIBOVRPLATFORM_INCLUDE_DIRS LIBOVRPLATFORM_LIBRARIES)

  mark_as_advanced(LIBOVRPLATFORM_INCLUDE_DIRS LIBOVRPLATFORM_LIBRARIES LIBOVRPLATFORM_SEARCH_DIRS)
endif ()
