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
  
  if (${TARGET_NAME}_DEPENDENCY_QT_MODULES)
    list(REMOVE_DUPLICATES ${TARGET_NAME}_DEPENDENCY_QT_MODULES)
    
    message(${TARGET_NAME})
    message(${${TARGET_NAME}_DEPENDENCY_QT_MODULES})
    
    # find these Qt modules and link them to our own target
    find_package(Qt5 COMPONENTS ${${TARGET_NAME}_DEPENDENCY_QT_MODULES} REQUIRED)
  
    foreach(QT_MODULE ${QT_MODULES_TO_LINK})
      target_link_libraries(${TARGET_NAME} Qt5::${QT_MODULE})
    endforeach()
  endif ()
  
  # we've already linked our Qt modules, but we need to bubble them up to parents
  list(APPEND ${TARGET_NAME}_LIBRARIES_TO_LINK "${${TARGET_NAME}_QT_MODULES_TO_LINK}")
  
  # set the property on this target so it can be retreived by targets linking to us
  set_target_properties(${TARGET_NAME} PROPERTIES DEPENDENCY_LIBRARIES "${${TARGET_NAME}_LIBRARIES_TO_LINK}")
  set_target_properties(${TARGET_NAME} PROPERTIES DEPENDENCY_INCLUDES "${${TARGET_NAME}_DEPENDENCY_INCLUDES}")
  set_target_properties(${TARGET_NAME} PROPERTIES DEPENDENCY_QT_MODULES "${${TARGET_NAME}_DEPENDENCY_QT_MODULES}")
endmacro(LINK_SHARED_DEPENDENCIES)