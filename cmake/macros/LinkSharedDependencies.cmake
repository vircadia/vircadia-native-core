# 
#  LinkSharedDependencies.cmake
#  cmake/macros
# 
#  Copyright 2014 High Fidelity, Inc.
#  Created by Stephen Birarda on August 8, 2014
# 
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(LINK_SHARED_DEPENDENCIES)  
  if (${TARGET_NAME}_LIBRARIES_TO_LINK)
    list(REMOVE_DUPLICATES ${TARGET_NAME}_LIBRARIES_TO_LINK)
    
    # link these libraries to our target
    target_link_libraries(${TARGET_NAME} ${${TARGET_NAME}_LIBRARIES_TO_LINK})
  endif ()
  
  if (${TARGET_NAME}_DEPENDENCY_INCLUDES)
    list(REMOVE_DUPLICATES ${TARGET_NAME}_DEPENDENCY_INCLUDES)
    
    # include those in our own target
    include_directories(SYSTEM ${${TARGET_NAME}_DEPENDENCY_INCLUDES})
  endif ()
  
  # we've already linked our Qt modules, but we need to bubble them up to parents
  list(APPEND ${TARGET_NAME}_LIBRARIES_TO_LINK "${${TARGET}_QT_MODULES_TO_LINK}")
  
  # set the property on this target so it can be retreived by targets linking to us
  set_target_properties(${TARGET_NAME} PROPERTIES DEPENDENCY_LIBRARIES "${${TARGET_NAME}_LIBRARIES_TO_LINK}")
  set_target_properties(${TARGET_NAME} PROPERTIES DEPENDENCY_INCLUDES "${${TARGET_NAME}_DEPENDENCY_INCLUDES}")
endmacro(LINK_SHARED_DEPENDENCIES)