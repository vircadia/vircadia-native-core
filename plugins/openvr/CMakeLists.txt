#
#  Created by Bradley Austin Davis on 2015/11/18
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

if (WIN32)
    # we're using static GLEW, so define GLEW_STATIC
    add_definitions(-DGLEW_STATIC)
    set(TARGET_NAME openvr)
    setup_hifi_plugin(OpenGL Script Qml Widgets)
    link_hifi_libraries(shared gl networking controllers ui 
        plugins display-plugins ui-plugins input-plugins script-engine
        render-utils model gpu render model-networking fbx)

    include_hifi_library_headers(octree)

    add_dependency_external_projects(OpenVR)
    find_package(OpenVR REQUIRED)
    target_include_directories(${TARGET_NAME} PRIVATE ${OPENVR_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${OPENVR_LIBRARIES})
endif()
