# 
#  SetupExternalProject.cmake
#  cmake/macros
# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Stephen Birarda on February 13, 2014
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(ADD_DEPENDENCY_EXTERNAL_PROJECT _PROJ_NAME)
  if (NOT TARGET ${_PROJ_NAME})
    
    if (ANDROID)
      set(_PROJ_BINARY_DIR ${EXTERNAL_PROJECT_DIR}/${_PROJ_NAME}/build/android)
    else ()
      set(_PROJ_BINARY_DIR ${EXTERNAL_PROJECT_DIR}/${_PROJ_NAME}/build)
    endif ()
    
    add_subdirectory(${EXTERNAL_PROJECT_DIR}/${_PROJ_NAME} ${_PROJ_BINARY_DIR})
  endif ()
  
  string(TOUPPER ${_PROJ_NAME} _PROJ_NAME_UPPER)
  get_target_property(${_PROJ_NAME_UPPER}_INCLUDE_DIRS ${_PROJ_NAME} INCLUDE_DIRS)
  
  add_dependencies(${TARGET_NAME} ${_PROJ_NAME})
endmacro()