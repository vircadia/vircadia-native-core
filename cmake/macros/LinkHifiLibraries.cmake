# 
#  LinkHifiLibrary.cmake
# 
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(LINK_HIFI_LIBRARIES)
  
  file(RELATIVE_PATH RELATIVE_LIBRARY_DIR_PATH ${CMAKE_CURRENT_SOURCE_DIR} "${HIFI_LIBRARY_DIR}")
  
  set(LIBRARIES_TO_LINK ${ARGN})
  
  foreach(HIFI_LIBRARY ${LIBRARIES_TO_LINK})    
    if (NOT TARGET ${HIFI_LIBRARY})
      add_subdirectory("${RELATIVE_LIBRARY_DIR_PATH}/${HIFI_LIBRARY}" "${RELATIVE_LIBRARY_DIR_PATH}/${HIFI_LIBRARY}")
    endif ()
  
    include_directories("${HIFI_LIBRARY_DIR}/${HIFI_LIBRARY}/src")

    add_dependencies(${TARGET_NAME} ${HIFI_LIBRARY})
  
    # link the actual library - it is static so don't bubble it up
    target_link_libraries(${TARGET_NAME} ${HIFI_LIBRARY})
    
    # ask the library what its dynamic dependencies are and link them
    get_target_property(LINKED_TARGET_DEPENDENCY_LIBRARIES ${HIFI_LIBRARY} DEPENDENCY_LIBRARIES)
    list(APPEND ${TARGET_NAME}_LIBRARIES_TO_LINK ${LINKED_TARGET_DEPENDENCY_LIBRARIES})
    
  endforeach()
  
endmacro(LINK_HIFI_LIBRARIES)