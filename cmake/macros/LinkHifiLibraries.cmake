# 
#  LinkHifiLibrary.cmake
# 
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(LINK_HIFI_LIBRARIES TARGET LIBRARIES)
  
  file(RELATIVE_PATH RELATIVE_LIBRARY_DIR_PATH ${CMAKE_CURRENT_SOURCE_DIR} "${HIFI_LIBRARY_DIR}")
  
  foreach(HIFI_LIBRARY ${LIBRARIES})
    
    if (NOT TARGET ${HIFI_LIBRARY})
      add_subdirectory("${RELATIVE_LIBRARY_DIR_PATH}/${HIFI_LIBRARY}" "${RELATIVE_LIBRARY_DIR_PATH}/${HIFI_LIBRARY}")
    endif ()
  
    include_directories("${HIFI_LIBRARY_DIR}/${HIFI_LIBRARY}/src")

    add_dependencies(${TARGET} ${HIFI_LIBRARY})
  
    get_target_property(LINKED_TARGET_DEPENDENCY_LIBRARIES ${HIFI_LIBRARY} DEPENDENCY_LIBRARIES)
  
    if (LINKED_TARGET_DEPENDENCY_LIBRARIES)
      target_link_libraries(${TARGET} ${HIFI_LIBRARY} ${LINKED_TARGET_DEPENDENCY_LIBRARIES})
    else ()
      target_link_libraries(${TARGET} ${HIFI_LIBRARY})
    endif ()
    
  endforeach()
  
endmacro(LINK_HIFI_LIBRARIES _target _libraries)