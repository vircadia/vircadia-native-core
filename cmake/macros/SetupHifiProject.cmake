# 
#  SetupHifiProject.cmake
# 
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(SETUP_HIFI_PROJECT TARGET)
  project(${TARGET})
  
  # grab the implemenation and header files
  file(GLOB TARGET_SRCS src/*)
  
  file(GLOB SRC_SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/*)
  
  foreach(DIR ${SRC_SUBDIRS})
    if (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/${DIR}")
      file(GLOB DIR_CONTENTS "src/${DIR}/*")
      set(TARGET_SRCS ${TARGET_SRCS} "${DIR_CONTENTS}")
    endif ()
  endforeach()
  
  # add the executable, include additional optional sources
  add_executable(${TARGET} ${TARGET_SRCS} "${AUTOMTC_SRC}")
  
  set(QT_MODULES_TO_LINK ${ARGN})
  list(APPEND QT_MODULES_TO_LINK Core)
  
  find_package(Qt5 COMPONENTS ${QT_MODULES_TO_LINK})
  
  foreach(QT_MODULE ${QT_MODULES_TO_LINK})    
    target_link_libraries(${TARGET} Qt5::${QT_MODULE})
    
    # add the actual path to the Qt module to our LIBRARIES_TO_LINK variable
    get_target_property(QT_LIBRARY_LOCATION Qt5::${QT_MODULE} LOCATION)
    list(APPEND ${TARGET}_QT_MODULES_TO_LINK ${QT_LIBRARY_LOCATION})
  endforeach()
  
endmacro()