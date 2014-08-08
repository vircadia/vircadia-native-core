# 
#  LinkHifiLibrary.cmake
# 
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(LINK_HIFI_LIBRARY LIBRARY TARGET)
  
  file(RELATIVE_PATH RELATIVE_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR} "${HIFI_LIBRARY_DIR}/${LIBRARY}")
  
  if (NOT TARGET ${LIBRARY})
    add_subdirectory(${RELATIVE_LIBRARY_PATH} ${RELATIVE_LIBRARY_PATH})
  endif ()
  
  include_directories("${HIFI_LIBRARY_DIR}/${LIBRARY}/src")

  add_dependencies(${TARGET} ${LIBRARY})
  
  get_target_property(LINKED_TARGET_DEPENDENCY_LIBRARIES ${LIBRARY} DEPENDENCY_LIBRARIES)
  
  if (LINKED_TARGET_DEPENDENCY_LIBRARIES)
    target_link_libraries(${TARGET} ${LIBRARY} ${LINKED_TARGET_DEPENDENCY_LIBRARIES})
  else ()
    target_link_libraries(${TARGET} ${LIBRARY})
  endif ()
  
endmacro(LINK_HIFI_LIBRARY _library _target)