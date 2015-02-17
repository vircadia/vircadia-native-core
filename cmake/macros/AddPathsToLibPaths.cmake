# 
#  AddPathsToLibPaths.cmake
#  cmake/macros
# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Stephen Birarda on February 17, 2014
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(ADD_PATHS_TO_LIB_PATHS)
  message(STATUS "ADD_PATHS called with ${ARGN}")
  foreach(_PATH ${ARGN})
    message(STATUS "ADDING ${_PATH}")
    message(STATUS "CURRENT LIB_PATHS is ${LIB_PATHS}")
    set(TEMP_LIB_PATHS ${LIB_PATHS})
    list(APPEND TEMP_LIB_PATHS ${_PATH})
    
    set(LIB_PATHS ${TEMP_LIB_PATHS} CACHE TYPE LIST FORCE)
    message(STATUS "LIB PATHS IS NOW ${LIB_PATHS}")
  endforeach()
endmacro()