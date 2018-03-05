#
#  Created by David Rowe on 15 Jun 2017.
#  Copyright 2017 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

find_package(LEAPMOTION)
if (LEAPMOTION_FOUND) 
    set(TARGET_NAME hifiLeapMotion)
    setup_hifi_plugin(Qml)
    link_hifi_libraries(shared controllers ui plugins input-plugins)
    target_leapmotion()
endif()
