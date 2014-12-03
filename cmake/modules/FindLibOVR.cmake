# 
#  FindLibOVR.cmake
# 
#  Try to find the LibOVR library to use the Oculus
#
#  You must provide a LIBOVR_ROOT_DIR which contains Lib and Include directories
#
#  Once done this will define
#
#  LIBOVR_FOUND - system found LibOVR
#  LIBOVR_INCLUDE_DIRS - the LibOVR include directory
#  LIBOVR_LIBRARIES - Link this to use LibOVR
#
#  Created on 5/9/2013 by Stephen Birarda
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("libovr")

find_path(LIBOVR_INCLUDE_DIRS OVR.h PATH_SUFFIXES Include HINTS ${LIBOVR_SEARCH_DIRS})
find_path(LIBOVR_SRC_DIR Util_Render_Stereo.h PATH_SUFFIXES Src/Util HINTS ${LIBOVR_SEARCH_DIRS})

include(SelectLibraryConfigurations)

if (APPLE)
  find_library(LIBOVR_LIBRARY_DEBUG NAMES ovr PATH_SUFFIXES Lib/Mac/Debug HINTS ${LIBOVR_SEARCH_DIRS})
  find_library(LIBOVR_LIBRARY_RELEASE NAMES ovr PATH_SUFFIXES Lib/Mac/Release HINTS ${LIBOVR_SEARCH_DIRS})
  find_library(ApplicationServices ApplicationServices)
  find_library(IOKit IOKit)
elseif (UNIX)
  find_library(UDEV_LIBRARY_RELEASE udev /usr/lib/x86_64-linux-gnu/)
  find_library(XINERAMA_LIBRARY_RELEASE Xinerama /usr/lib/x86_64-linux-gnu/)
  
  if (CMAKE_CL_64)
    set(LINUX_ARCH_DIR "i386")
  else()
    set(LINUX_ARCH_DIR "x86_64")
  endif()
  
  find_library(LIBOVR_LIBRARY_DEBUG NAMES ovr PATH_SUFFIXES Lib/Linux/Debug/${LINUX_ARCH_DIR} HINTS ${LIBOVR_SEARCH_DIRS})
  find_library(LIBOVR_LIBRARY_RELEASE NAMES ovr PATH_SUFFIXES Lib/Linux/Release/${LINUX_ARCH_DIR} HINTS ${LIBOVR_SEARCH_DIRS})
  
  select_library_configurations(UDEV)
  select_library_configurations(XINERAMA)
  
elseif (WIN32)   
  if (MSVC10)
    find_library(LIBOVR_LIBRARY_DEBUG NAMES libovrd PATH_SUFFIXES Lib/Win32/VS2010 HINTS ${LIBOVR_SEARCH_DIRS})
    find_library(LIBOVR_LIBRARY_RELEASE NAMES libovr PATH_SUFFIXES Lib/Win32/VS2010 HINTS ${LIBOVR_SEARCH_DIRS})
  elseif (MSVC12)
    find_library(LIBOVR_LIBRARY_DEBUG NAMES libovrd PATH_SUFFIXES Lib/Win32/VS2013 HINTS ${LIBOVR_SEARCH_DIRS})
    find_library(LIBOVR_LIBRARY_RELEASE NAMES libovr PATH_SUFFIXES Lib/Win32/VS2013 HINTS ${LIBOVR_SEARCH_DIRS})
  endif ()
  find_package(ATL)
endif ()

select_library_configurations(LIBOVR)
set(LIBOVR_LIBRARIES ${LIBOVR_LIBRARY})

list(APPEND LIBOVR_ARGS_LIST LIBOVR_INCLUDE_DIRS LIBOVR_SRC_DIR LIBOVR_LIBRARY)

if (APPLE)
  list(APPEND LIBOVR_LIBRARIES ${IOKit} ${ApplicationServices})
  list(APPEND LIBOVR_ARGS_LIST IOKit ApplicationServices)
elseif (UNIX) 
  list(APPEND LIBOVR_LIBRARIES "${UDEV_LIBRARY}" "${XINERAMA_LIBRARY}")
  list(APPEND LIBOVR_ARGS_LIST UDEV_LIBRARY XINERAMA_LIBRARY)
elseif (WIN32)
  list(APPEND LIBOVR_LIBRARIES ${ATL_LIBRARIES})
  list(APPEND LIBOVR_ARGS_LIST ATL_LIBRARIES)
endif ()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LibOVR DEFAULT_MSG ${LIBOVR_ARGS_LIST})

mark_as_advanced(LIBOVR_INCLUDE_DIRS LIBOVR_LIBRARIES LIBOVR_SEARCH_DIRS)
