# 
#  LinkHifiLibrary.cmake
# 
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(LINK_HIFI_LIBRARY LIBRARY TARGET ROOT_DIR)
  
  if (NOT TARGET ${LIBRARY})
    add_subdirectory("${ROOT_DIR}/libraries/${LIBRARY}" "${ROOT_DIR}/libraries/${LIBRARY}")
  endif ()
  
  include_directories("${ROOT_DIR}/libraries/${LIBRARY}/src")

  add_dependencies(${TARGET} ${LIBRARY})
  
  get_target_property(LINKED_TARGET_DEPENDENCY_LIBRARIES ${LIBRARY} DEPENDENCY_LIBRARIES)
  
  if (LINKED_TARGET_DEPENDENCY_LIBRARIES)
    target_link_libraries(${TARGET} ${LIBRARY} ${LINKED_TARGET_DEPENDENCY_LIBRARIES})
  else ()
    target_link_libraries(${TARGET} ${LIBRARY})
  endif ()
  
endmacro(LINK_HIFI_LIBRARY _library _target _root_dir)