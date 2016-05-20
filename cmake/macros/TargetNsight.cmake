#
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#
macro(TARGET_NSIGHT)
    if (WIN32 AND USE_NSIGHT)
    
      # grab the global CHECKED_FOR_NSIGHT_ONCE property
      get_property(NSIGHT_UNAVAILABLE GLOBAL PROPERTY CHECKED_FOR_NSIGHT_ONCE)

      if (NOT NSIGHT_UNAVAILABLE)
        # try to find the Nsight package and add it to the build if we find it
        find_package(NSIGHT)

        # Cache the failure to find nsight, so that we don't check over and over
        if (NOT NSIGHT_FOUND)
            set_property(GLOBAL PROPERTY CHECKED_FOR_NSIGHT_ONCE TRUE)
        endif()
      endif ()
    
      # try to find the Nsight package and add it to the build if we find it
      if (NSIGHT_FOUND)
        include_directories(${NSIGHT_INCLUDE_DIRS})
        add_definitions(-DNSIGHT_FOUND)
        target_link_libraries(${TARGET_NAME} "${NSIGHT_LIBRARIES}")
      endif ()
    endif ()
endmacro()
