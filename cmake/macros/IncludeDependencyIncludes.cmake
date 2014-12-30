# 
#  IncludeDependencyIncludes.cmake
#  cmake/macros
# 
#  Copyright 2014 High Fidelity, Inc.
#  Created by Stephen Birarda on August 8, 2014
# 
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(INCLUDE_DEPENDENCY_INCLUDES)
  if (${TARGET_NAME}_DEPENDENCY_INCLUDES)
    list(REMOVE_DUPLICATES ${TARGET_NAME}_DEPENDENCY_INCLUDES)
    
    # include those in our own target
    include_directories(SYSTEM ${${TARGET_NAME}_DEPENDENCY_INCLUDES})
  endif ()
  
  # set the property on this target so it can be retreived by targets linking to us
  set_target_properties(${TARGET_NAME} PROPERTIES DEPENDENCY_INCLUDES "${${TARGET_NAME}_DEPENDENCY_INCLUDES}")
endmacro(INCLUDE_DEPENDENCY_INCLUDES)