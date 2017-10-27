#
#  FindTBB.cmake
#
#  Try to find the Intel Threading Building Blocks library
#
#  You can provide a TBB_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  TBB_FOUND - system was able to find TBB
#  TBB_INCLUDE_DIRS - the TBB include directory
#  TBB_LIBRARIES - link this to use TBB
#
#  Created on 12/14/2014 by Stephen Birarda
#  Copyright 2014 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("tbb")

find_path(TBB_INCLUDE_DIRS tbb/tbb.h PATH_SUFFIXES include HINTS ${TBB_SEARCH_DIRS})

set(_TBB_LIB_NAME "tbb")
set(_TBB_LIB_MALLOC_NAME "${_TBB_LIB_NAME}malloc")

if (APPLE)
  set(_TBB_LIB_DIR "lib/libc++")
elseif (UNIX AND NOT ANDROID)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_TBB_ARCH_DIR "intel64")
  else()
    set(_TBB_ARCH_DIR "ia32")
  endif()

  execute_process(
    COMMAND ${CMAKE_C_COMPILER} -dumpversion
    OUTPUT_VARIABLE GCC_VERSION
  )

  if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)
    set(_TBB_LIB_DIR "lib/${_TBB_ARCH_DIR}/gcc4.7")
  elseif (GCC_VERSION VERSION_GREATER 4.4 OR GCC_VERSION VERSION_EQUAL 4.4)
    set(_TBB_LIB_DIR "lib/${_TBB_ARCH_DIR}/gcc4.4")
  elseif (GCC_VERSION VERSION_GREATER 4.1 OR GCC_VERSION VERSION_EQUAL 4.1)
    set(_TBB_LIB_DIR "lib/${_TBB_ARCH_DIR}/gcc4.1")
  else ()
    message(FATAL_ERROR "Could not find a compatible version of Threading Building Blocks library for your compiler.")
  endif ()

elseif (WIN32)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_TBB_ARCH_DIR "intel64")
  else()
    set(_TBB_ARCH_DIR "ia32")
  endif()

  if (MSVC_VERSION GREATER_EQUAL 1900)
    set(_TBB_MSVC_DIR "vc14")
  elseif (MSVC_VERSION GREATER_EQUAL 1800)
    set(_TBB_MSVC_DIR "vc12")
  elseif (MSVC_VERSION GREATER_EQUAL 1700)
    set(_TBB_MSVC_DIR "vc11")
  else()
    message(FATAL_ERROR "MSVC ${MSVC_VERSION} not supported by Intel TBB")
  endif()

  set(_TBB_LIB_DIR "lib/${_TBB_ARCH_DIR}/${_TBB_MSVC_DIR}")

  find_path(TBB_DLL_PATH tbb_debug.dll PATH_SUFFIXES "bin/${_TBB_ARCH_DIR}/${_TBB_MSVC_DIR}" HINTS ${TBB_SEARCH_DIRS})

elseif (ANDROID)
  set(_TBB_DEFAULT_INSTALL_DIR "/tbb")
  set(_TBB_LIB_NAME "tbb")
  set(_TBB_LIB_DIR "lib")
  set(_TBB_LIB_MALLOC_NAME "${_TBB_LIB_NAME}malloc")
  set(_TBB_LIB_DEBUG_NAME "${_TBB_LIB_NAME}_debug")
  set(_TBB_LIB_MALLOC_DEBUG_NAME "${_TBB_LIB_MALLOC_NAME}_debug")
endif ()

find_library(TBB_LIBRARY_DEBUG NAMES ${_TBB_LIB_NAME}_debug PATH_SUFFIXES ${_TBB_LIB_DIR} HINTS ${TBB_SEARCH_DIRS})
find_library(TBB_LIBRARY_RELEASE NAMES ${_TBB_LIB_NAME} PATH_SUFFIXES ${_TBB_LIB_DIR} HINTS ${TBB_SEARCH_DIRS})

find_library(TBB_MALLOC_LIBRARY_DEBUG NAMES ${_TBB_LIB_MALLOC_NAME}_debug PATH_SUFFIXES ${_TBB_LIB_DIR} HINTS ${TBB_SEARCH_DIRS})
find_library(TBB_MALLOC_LIBRARY_RELEASE NAMES ${_TBB_LIB_MALLOC_NAME} PATH_SUFFIXES ${_TBB_LIB_DIR} HINTS ${TBB_SEARCH_DIRS})

include(SelectLibraryConfigurations)
include(FindPackageHandleStandardArgs)

select_library_configurations(TBB)
select_library_configurations(TBB_MALLOC)

set(TBB_REQUIREMENTS TBB_LIBRARY TBB_MALLOC_LIBRARY TBB_INCLUDE_DIRS)
if (WIN32)
  list(APPEND TBB_REQUIREMENTS TBB_DLL_PATH)
endif ()

find_package_handle_standard_args(TBB DEFAULT_MSG ${TBB_REQUIREMENTS})

if (WIN32)
  add_paths_to_fixup_libs(${TBB_DLL_PATH})
endif ()

set(TBB_LIBRARIES ${TBB_LIBRARY} ${TBB_MALLOC_LIBRARY})
