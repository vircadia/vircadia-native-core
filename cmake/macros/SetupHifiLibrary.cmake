# 
#  SetupHifiLibrary.cmake
# 
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(SETUP_HIFI_LIBRARY)
  
  project(${TARGET_NAME})

  # grab the implemenation and header files
  file(GLOB_RECURSE LIB_SRCS "src/*.h" "src/*.cpp")
  set(LIB_SRCS ${LIB_SRCS})

  # create a library and set the property so it can be referenced later
  add_library(${TARGET_NAME} ${LIB_SRCS} ${AUTOMTC_SRC} ${AUTOSCRIBE_SHADER_LIB_SRC})
  
  set(${TARGET_NAME}_DEPENDENCY_QT_MODULES ${ARGN})
  list(APPEND ${TARGET_NAME}_DEPENDENCY_QT_MODULES Core)
  
  # find these Qt modules and link them to our own target
  find_package(Qt5 COMPONENTS ${${TARGET_NAME}_DEPENDENCY_QT_MODULES} REQUIRED)

  foreach(QT_MODULE ${${TARGET_NAME}_DEPENDENCY_QT_MODULES})
    target_link_libraries(${TARGET_NAME} Qt5::${QT_MODULE})
  endforeach()
  
endmacro(SETUP_HIFI_LIBRARY)