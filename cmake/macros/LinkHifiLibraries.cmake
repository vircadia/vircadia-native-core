# 
#  LinkHifiLibrary.cmake
# 
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

function(LINK_HIFI_LIBRARIES)
  
  file(RELATIVE_PATH RELATIVE_LIBRARY_DIR_PATH ${CMAKE_CURRENT_SOURCE_DIR} "${HIFI_LIBRARY_DIR}")
  
  set(LIBRARIES_TO_LINK ${ARGN})
  foreach(HIFI_LIBRARY ${LIBRARIES_TO_LINK})
    if (NOT TARGET ${HIFI_LIBRARY})
      add_subdirectory("${RELATIVE_LIBRARY_DIR_PATH}/${HIFI_LIBRARY}" "${RELATIVE_LIBRARY_DIR_PATH}/${HIFI_LIBRARY}")
    endif ()
  endforeach()

  foreach(HIFI_LIBRARY ${LIBRARIES_TO_LINK})
    target_include_directories(${TARGET_NAME} PRIVATE "${HIFI_LIBRARY_DIR}/${HIFI_LIBRARY}/src")
    target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_BINARY_DIR}/libraries/${HIFI_LIBRARY}")
    # link the actual library - it is static so don't bubble it up
    target_link_libraries(${TARGET_NAME} ${HIFI_LIBRARY})
  endforeach()

  setup_memory_debugger()
  setup_thread_debugger()

endfunction()
