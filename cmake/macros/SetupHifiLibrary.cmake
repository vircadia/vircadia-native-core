# 
#  SetupHifiLibrary.cmake
# 
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(SETUP_HIFI_LIBRARY TARGET)
  
  project(${TARGET})

  # grab the implemenation and header files
  file(GLOB LIB_SRCS src/*.h src/*.cpp)
  set(LIB_SRCS ${LIB_SRCS} ${WRAPPED_SRCS})

  # create a library and set the property so it can be referenced later
  add_library(${TARGET} ${LIB_SRCS} ${ARGN})
  
  find_package(Qt5Core REQUIRED)
  qt5_use_modules(${TARGET} Core)

  target_link_libraries(${TARGET} ${QT_LIBRARIES})

endmacro(SETUP_HIFI_LIBRARY _target)