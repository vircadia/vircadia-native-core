# - Try to find the Bullet physics engine
#
#  This module defines the following variables
#
#  BULLET_FOUND - Was bullet found
#  BULLET_INCLUDE_DIRS - the Bullet include directories
#  BULLET_LIBRARIES - Link to this, by default it includes
#                     all bullet components (Dynamics,
#                     Collision, LinearMath, & SoftBody)
#
#  This module accepts the following variables
#
#  BULLET_ROOT - Can be set to bullet install path or Windows build path
#
# Modified on 2015.01.15 by Andrew Meadows
# This is an adapted version of the FindBullet.cmake module distributed with Cmake 2.8.12.2
# The original license for that file is displayed below.
#
#=============================================================================
# Copyright 2009 Kitware, Inc.
# Copyright 2009 Philip Lowman <philip@yhbt.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)


include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("bullet")

macro(_FIND_BULLET_LIBRARY _var)
  set(_${_var}_NAMES ${ARGN})
  find_library(${_var}_LIBRARY_RELEASE
     NAMES ${_${_var}_NAMES}
     HINTS
        ${BULLET_SEARCH_DIRS}
        $ENV{BULLET_ROOT_DIR}
        ${BULLET_ROOT}
        PATH_SUFFIXES lib lib/Release out/release8/libs
  )
  
  foreach(_NAME IN LISTS _${_var}_NAMES)
    list(APPEND _${_var}_DEBUG_NAMES "${_NAME}_Debug")
    list(APPEND _${_var}_DEBUG_NAMES "${_NAME}_d")
  endforeach()
  
  find_library(${_var}_LIBRARY_DEBUG
     NAMES ${_${_var}_DEBUG_NAMES}
     HINTS
        ${BULLET_SEARCH_DIRS}
        $ENV{BULLET_ROOT_DIR}
        ${BULLET_ROOT}
        PATH_SUFFIXES lib lib/Debug out/debug8/libs
  )
  
  select_library_configurations(${_var})
  
  mark_as_advanced(${_var}_LIBRARY)
  mark_as_advanced(${_var}_LIBRARY)
endmacro()

find_path(BULLET_INCLUDE_DIR NAMES btBulletCollisionCommon.h
  HINTS
    ${BULLET_SEARCH_DIRS}/include
    $ENV{BULLET_ROOT_DIR}
    ${BULLET_ROOT}/include
    ${BULLET_ROOT}/src
  PATH_SUFFIXES bullet
)

# Find the libraries

_FIND_BULLET_LIBRARY(BULLET_DYNAMICS BulletDynamics)
_FIND_BULLET_LIBRARY(BULLET_COLLISION BulletCollision)
_FIND_BULLET_LIBRARY(BULLET_MATH BulletMath LinearMath)
_FIND_BULLET_LIBRARY(BULLET_SOFTBODY BulletSoftBody)

set(BULLET_INCLUDE_DIRS ${BULLET_INCLUDE_DIR})
set(BULLET_LIBRARIES ${BULLET_DYNAMICS_LIBRARY} ${BULLET_COLLISION_LIBRARY} ${BULLET_MATH_LIBRARY} ${BULLET_SOFTBODY_LIBRARY})

find_package_handle_standard_args(Bullet "Could NOT find Bullet, try to set the path to Bullet root folder in the system variable BULLET_ROOT_DIR"
    BULLET_INCLUDE_DIRS
    BULLET_DYNAMICS_LIBRARY BULLET_COLLISION_LIBRARY BULLET_MATH_LIBRARY BULLET_SOFTBODY_LIBRARY
    BULLET_LIBRARIES
)
