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
  
  string(TOUPPER ${_PROJ_NAME} _PROJ_NAME_UPPER)
  
  if (NOT DEFINED GET_${_PROJ_NAME_UPPER} OR GET_${_PROJ_NAME_UPPER})
    if (NOT TARGET ${_PROJ_NAME})
    
      if (ANDROID)
        set(_PROJ_BINARY_DIR ${EXTERNAL_PROJECT_DIR}/${_PROJ_NAME}/build/android)
      else ()
        set(_PROJ_BINARY_DIR ${EXTERNAL_PROJECT_DIR}/${_PROJ_NAME}/build)
      endif ()
    
      add_subdirectory(${EXTERNAL_PROJECT_DIR}/${_PROJ_NAME} ${_PROJ_BINARY_DIR})
    endif ()
  
    add_dependencies(${TARGET_NAME} ${_PROJ_NAME})
    
  endif ()
  
endmacro()