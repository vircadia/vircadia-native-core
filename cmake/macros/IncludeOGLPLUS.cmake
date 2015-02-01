# 
#  IncludeOGLPLUS.cmake
# 
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(INCLUDE_OGLPLUS)
  find_package(OGLPLUS REQUIRED)
  find_package(BOOSTCONFIG REQUIRED)
  include_directories("${OGLPLUS_INCLUDE_DIRS}")
  include_directories("${BOOSTCONFIG_INCLUDE_DIRS}")

  if (APPLE OR UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${OGLPLUS_INCLUDE_DIRS}")
  endif ()

  # force oglplus to work in header only mode, use GLEW for GL definitions
  # and use boostconfig to determine compiler feature support.  
  add_definitions(-DOGLPLUS_NO_SITE_CONFIG=1)
  add_definitions(-DOGLPLUS_USE_GLEW=1)
  add_definitions(-DOGLPLUS_USE_GLCOREARB=0)
  add_definitions(-DOGLPLUS_USE_BOOST_CONFIG=1)

endmacro(INCLUDE_OGLPLUS)