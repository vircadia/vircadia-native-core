#
#  Created by Brad Hefta-Gaub on 2016/12/7
#  Copyright 2016 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#
macro(TARGET_KINECT)
    # Kinect SDK data reader is only available on these platforms
    if (WIN32)
        #add_dependency_external_projects(kinect)
        find_package(Kinect REQUIRED)
        target_include_directories(${TARGET_NAME} PRIVATE ${KINECT_INCLUDE_DIRS})
        target_link_libraries(${TARGET_NAME} ${KINECT_LIBRARIES})
        add_definitions(-DHAVE_KINECT)
    endif(WIN32)
endmacro()
