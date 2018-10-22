#
#  ConfigureCCache.cmake
#  cmake/macros
#
#  Created by Clement Brisset on 10/10/18.
#  Copyright 2018 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

macro(configure_ccache)
  find_program(CCACHE_PROGRAM ccache)
  if(CCACHE_PROGRAM)
    message(STATUS "Configuring ccache")

    # Set up wrapper scripts
    set(C_LAUNCHER   "${CCACHE_PROGRAM}")
    set(CXX_LAUNCHER "${CCACHE_PROGRAM}")
    
    set(LAUNCH_C_IN "${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/launch-c.in")
    set(LAUNCH_CXX_IN "${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/launch-cxx.in")
    set(LAUNCH_C "${CMAKE_BINARY_DIR}/CMakeFiles/launch-c")
    set(LAUNCH_CXX "${CMAKE_BINARY_DIR}/CMakeFiles/launch-cxx")

    configure_file(${LAUNCH_C_IN} ${LAUNCH_C})
    configure_file(${LAUNCH_CXX_IN} ${LAUNCH_CXX})
    execute_process(COMMAND chmod a+rx ${LAUNCH_C} ${LAUNCH_CXX})

    if(CMAKE_GENERATOR STREQUAL "Xcode")
      # Set Xcode project attributes to route compilation and linking
      # through our scripts
      set(CMAKE_XCODE_ATTRIBUTE_CC ${LAUNCH_C})
      set(CMAKE_XCODE_ATTRIBUTE_CXX ${LAUNCH_CXX})
      set(CMAKE_XCODE_ATTRIBUTE_LD ${LAUNCH_C})
      set(CMAKE_XCODE_ATTRIBUTE_LDPLUSPLUS ${LAUNCH_CXX})
    else()
      # Support Unix Makefiles and Ninja
      set(CMAKE_C_COMPILER_LAUNCHER ${LAUNCH_C})
      set(CMAKE_CXX_COMPILER_LAUNCHER ${LAUNCH_CXX})
    endif()
  else()
    message(WARNING "Could not find ccache")
  endif()
endmacro()
