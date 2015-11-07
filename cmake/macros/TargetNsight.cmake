# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_NSIGHT)
    if (WIN32)
      if (USE_NSIGHT)
        # try to find the Nsight package and add it to the build if we find it
        find_package(NSIGHT)
        if (NSIGHT_FOUND)
          include_directories(${NSIGHT_INCLUDE_DIRS})
          add_definitions(-DNSIGHT_FOUND)
          target_link_libraries(${TARGET_NAME} "${NSIGHT_LIBRARIES}")
        endif ()
      endif()
    endif (WIN32)
endmacro()