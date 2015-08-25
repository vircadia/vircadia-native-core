# 
#  SetupHifiProject.cmake
# 
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(SETUP_HIFI_PROJECT)
  project(${TARGET_NAME})
  
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
  add_executable(${TARGET_NAME} ${TARGET_SRCS} ${AUTOMTC_SRC} ${AUTOSCRIBE_SHADER_LIB_SRC})
  
  if ("${AUTOSCRIBE_SHADER_LIB_SRC}" STRGREATER "")
    message("Adding autoscribe files to target '${TARGET_NAME}': ${AUTOSCRIBE_SHADER_LIB_SRC}")

    file(GLOB LIST_OF_HEADER_FILES_IN_CURRENT_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/*.h")
    if ("${LIST_OF_HEADER_FILES_IN_CURRENT_BUILD_DIR}" STRGREATER "")
      message("directory '${CMAKE_CURRENT_BINARY_DIR}' contains files: ${LIST_OF_HEADER_FILES_IN_CURRENT_BUILD_DIR}")
    else ()
      message("directory '${CMAKE_CURRENT_BINARY_DIR}' does not contain any header files")
    endif ()
  endif ()

  set(${TARGET_NAME}_DEPENDENCY_QT_MODULES ${ARGN})
  list(APPEND ${TARGET_NAME}_DEPENDENCY_QT_MODULES Core)
  
  # find these Qt modules and link them to our own target
  find_package(Qt5 COMPONENTS ${${TARGET_NAME}_DEPENDENCY_QT_MODULES} REQUIRED)

  foreach(QT_MODULE ${${TARGET_NAME}_DEPENDENCY_QT_MODULES})
    target_link_libraries(${TARGET_NAME} Qt5::${QT_MODULE})
  endforeach()
endmacro()
