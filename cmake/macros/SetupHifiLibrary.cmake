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
  set(LIB_SRCS ${LIB_SRCS})

  # create a library and set the property so it can be referenced later  
  add_library(${TARGET} ${LIB_SRCS} ${AUTOMTC_SRC})
  
  set(QT_MODULES_TO_LINK ${ARGN})
  list(APPEND QT_MODULES_TO_LINK Core)
  
  find_package(Qt5 COMPONENTS ${QT_MODULES_TO_LINK})
  
  foreach(QT_MODULE ${QT_MODULES_TO_LINK})
    # link this Qt module to ourselves
    target_link_libraries(${TARGET} Qt5::${QT_MODULE})
    
    get_target_property(QT_LIBRARY_LOCATION Qt5::${QT_MODULE} LOCATION)
    
    # add the actual path to the Qt module to a QT_LIBRARIES variable
    list(APPEND QT_LIBRARIES ${QT_LIBRARY_LOCATION})
  endforeach()
  
  if (QT_LIBRARIES)
    set_target_properties(${TARGET} PROPERTIES DEPENDENCY_LIBRARIES "${QT_LIBRARIES}")
  endif ()

endmacro(SETUP_HIFI_LIBRARY _target)