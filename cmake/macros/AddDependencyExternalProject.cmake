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
  add_subdirectory(${EXTERNAL_PROJECT_DIR}/${_PROJ_NAME} ${EXTERNAL_PROJECT_DIR}/${_PROJ_NAME}/build)
  add_dependencies(${TARGET_NAME} ${_PROJ_NAME})
endmacro()