#
#  Created by Bradley Austin Davis on 2016/05/11
#  Copyright 2013-2016 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

if(WIN32)
    set(TARGET_NAME hifiSpacemouse)
    find_package(3DCONNEXIONCLIENT)
    if (3DCONNEXIONCLIENT_FOUND) 
        setup_hifi_plugin(Script Qml Widgets)
        link_hifi_libraries(shared networking controllers ui plugins input-plugins)
        target_include_directories(${TARGET_NAME} PUBLIC ${3DCONNEXIONCLIENT_INCLUDE_DIRS})
        target_link_libraries(${TARGET_NAME} ${3DCONNEXIONCLIENT_LIBRARIES})
    endif()
endif()
