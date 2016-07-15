# 
#  AddDependencyExternalProjects.cmake
#  cmake/macros
# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Stephen Birarda on February 13, 2014
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(ADD_DEPENDENCY_EXTERNAL_PROJECTS)
  
  foreach(_PROJ_NAME ${ARGN})
    
    string(TOUPPER ${_PROJ_NAME} _PROJ_NAME_UPPER)
    
    # has the user told us they specific don't want this as an external project?
    if (NOT USE_LOCAL_${_PROJ_NAME_UPPER})
      message(STATUS "LEODEBUG: Looking for dependency ${_PROJ_NAME}")
      # have we already detected we can't have this as external project on this OS?
      if (NOT DEFINED ${_PROJ_NAME_UPPER}_EXTERNAL_PROJECT OR ${_PROJ_NAME_UPPER}_EXTERNAL_PROJECT)
        # have we already setup the target?
        if (NOT TARGET ${_PROJ_NAME})
          message(STATUS "LEODEBUG: We dont have a target with that name ${_PROJ_NAME} adding directory ${EXTERNAL_PROJECT_DIR}/${_PROJ_NAME}")
          add_subdirectory(${EXTERNAL_PROJECT_DIR}/${_PROJ_NAME} ${EXTERNALS_BINARY_DIR}/${_PROJ_NAME})
          
          # did we end up adding an external project target?
          if (NOT TARGET ${_PROJ_NAME})
            set(${_PROJ_NAME_UPPER}_EXTERNAL_PROJECT FALSE CACHE BOOL "Presence of ${_PROJ_NAME} as external target")
            
            message(STATUS "${_PROJ_NAME} was not added as an external project target for your OS."
                    " Either your system should already have the external library or you will need to install it separately.")
          else ()
            set(${_PROJ_NAME_UPPER}_EXTERNAL_PROJECT TRUE CACHE BOOL "Presence of ${_PROJ_NAME} as external target")
          endif ()
        endif ()
      
        if (TARGET ${_PROJ_NAME})
          message(STATUS "LEODEBUG: We no have the target ${_PROJ_NAME}")
          add_dependencies(${TARGET_NAME} ${_PROJ_NAME})
        endif ()
        
      endif ()
    endif ()
    
  endforeach()

endmacro()
