#  Created by Bradley Austin Davis on 2017/09/02
#  Copyright 2013-2017 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

function(get_sub_directories result curdir)
  file(GLOB children RELATIVE ${curdir} ${curdir}/*)
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${curdir}/${child})
      LIST(APPEND dirlist ${child})
    endif()
  endforeach()
  set(${result} ${dirlist} PARENT_SCOPE)
endfunction()

function(calculate_qt5_version result _QT_DIR)
  # All Qt5 packages have little "private" include directories named with the actual Qt version such as:
  #   .../include/QtCore/5.12.3/QtCore/private
  # Sometimes we need to include these private headers for debug hackery.
  # Hence we find one of these directories and pick apart its path to determine the actual QT_VERSION.
  if (APPLE)
    set(_QT_CORE_DIR "${_QT_DIR}/lib/QtCore.framework/Versions/5/Headers")
  else()
    set(_QT_CORE_DIR "${_QT_DIR}/include/QtCore")
  endif()
  if(NOT EXISTS "${_QT_CORE_DIR}")
      message(FATAL_ERROR "Could not find 'include/QtCore' in '${_QT_DIR}'")
  endif()
  set(subdirs "")
  get_sub_directories(subdirs ${_QT_CORE_DIR})

  foreach(subdir ${subdirs})
    string(REGEX MATCH "5.[0-9]+.[0-9]+$" _QT_VERSION ${subdir})
    if (NOT "${_QT_VERSION}" STREQUAL "")
      # found it!
      set(${result} "${_QT_VERSION}" PARENT_SCOPE)
      break()
    endif()
  endforeach()
endfunction()

# Sets the QT_CMAKE_PREFIX_PATH and QT_DIR variables
# Also enables CMAKE_AUTOMOC and CMAKE_AUTORCC
macro(setup_qt)
    # if we are in a development build and QT_CMAKE_PREFIX_PATH is specified
    # then use it,
    # otherwise, use the vcpkg'ed version
    if(NOT DEFINED QT_CMAKE_PREFIX_PATH)
        message(FATAL_ERROR "QT_CMAKE_PREFIX_PATH should have been set by hifi_qt.py")
    endif()
    if (DEV_BUILD)
      if (DEFINED ENV{QT_CMAKE_PREFIX_PATH})
        set(QT_CMAKE_PREFIX_PATH $ENV{QT_CMAKE_PREFIX_PATH})
      endif()
    endif()

    message("QT_CMAKE_PREFIX_PATH = " ${QT_CMAKE_PREFIX_PATH})

    # figure out where the qt dir is
    get_filename_component(QT_DIR "${QT_CMAKE_PREFIX_PATH}/../../" ABSOLUTE)
    set(QT_VERSION "unknown")
    calculate_qt5_version(QT_VERSION "${QT_DIR}")
    if (QT_VERSION STREQUAL "unknown")
      message(FATAL_ERROR "Could not determine QT_VERSION")
    endif()

    if(WIN32)
        # windows shell does not like backslashes expanded on the command line,
        # so convert all backslashes in the QT path to forward slashes
        string(REPLACE \\ / QT_CMAKE_PREFIX_PATH ${QT_CMAKE_PREFIX_PATH})
        string(REPLACE \\ / QT_DIR ${QT_DIR})
    endif()

    if(NOT EXISTS "${QT_CMAKE_PREFIX_PATH}/Qt5Core/Qt5CoreConfig.cmake")
        message(FATAL_ERROR "Unable to locate Qt5CoreConfig.cmake in '${QT_CMAKE_PREFIX_PATH}'")
    endif()

    message(STATUS "Using Qt build in : '${QT_DIR}' with version ${QT_VERSION}")

    # Instruct CMake to run moc automatically when needed.
    set(CMAKE_AUTOMOC ON)

    # Instruct CMake to run rcc automatically when needed
    set(CMAKE_AUTORCC ON)

    if (WIN32)
        add_paths_to_fixup_libs("${QT_DIR}/bin")
    endif ()

endmacro()
