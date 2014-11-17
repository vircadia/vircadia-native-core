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
  list(APPEND ${TARGET_NAME}_SRCS ${LIB_SRCS})

  # create a library and set the property so it can be referenced later
  add_library(${TARGET_NAME} ${${TARGET_NAME}_SRCS} ${AUTOMTC_SRC})
  
  set(QT_MODULES_TO_LINK ${ARGN})
  list(APPEND QT_MODULES_TO_LINK Core)
  
  find_package(Qt5 COMPONENTS ${QT_MODULES_TO_LINK} REQUIRED)
  
  foreach(QT_MODULE ${QT_MODULES_TO_LINK})
    get_target_property(QT_LIBRARY_LOCATION Qt5::${QT_MODULE} LOCATION)
    
    # add the actual path to the Qt module to our LIBRARIES_TO_LINK variable
    target_link_libraries(${TARGET_NAME} Qt5::${QT_MODULE})
    list(APPEND ${TARGET_NAME}_QT_MODULES_TO_LINK ${QT_LIBRARY_LOCATION})
  endforeach()
endmacro(SETUP_HIFI_LIBRARY)