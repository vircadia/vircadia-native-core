#
#  Created by Brad Hefta-Gaub on 2016/12/7
#  Copyright 2016 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

if (WIN32)
    find_package(KINECT)
    if (KINECT_FOUND) 
        set(TARGET_NAME hifiKinect)
        setup_hifi_plugin(Script Qml Widgets)
        link_hifi_libraries(shared controllers ui plugins input-plugins display-plugins)
    
        # need to setup appropriate externals...
        target_kinect()
    endif()
endif()
