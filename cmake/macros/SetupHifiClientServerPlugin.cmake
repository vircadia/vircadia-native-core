#
#  Created by Brad Hefta-Gaub on 2016/07/07
#  Copyright 2016 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#
macro(SETUP_HIFI_CLIENT_SERVER_PLUGIN)
    set(${TARGET_NAME}_SHARED 1)
    setup_hifi_library(${ARGV})
    add_dependencies(interface ${TARGET_NAME})
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Plugins")

    if (APPLE)
        set(PLUGIN_PATH "${INTERFACE_BUNDLE_NAME}.app/Contents/PlugIns")
    else()
        set(PLUGIN_PATH "plugins")
    endif()

    if (CMAKE_SYSTEM_NAME MATCHES "Linux" OR CMAKE_GENERATOR STREQUAL "Unix Makefiles")
        set(CLIENT_PLUGIN_FULL_PATH "${CMAKE_BINARY_DIR}/interface/${PLUGIN_PATH}/")
        set(SERVER_PLUGIN_FULL_PATH "${CMAKE_BINARY_DIR}/assignment-client/${PLUGIN_PATH}/")
    else()
        set(CLIENT_PLUGIN_FULL_PATH "${CMAKE_BINARY_DIR}/interface/$<CONFIGURATION>/${PLUGIN_PATH}/")
        set(SERVER_PLUGIN_FULL_PATH "${CMAKE_BINARY_DIR}/assignment-client/$<CONFIGURATION>/${PLUGIN_PATH}/")
    endif()

    # create the destination for the client plugin binaries
    add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
        ${CLIENT_PLUGIN_FULL_PATH}
    )
    # copy the client plugin binaries
    add_custom_command(TARGET ${DIR} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy
        "$<TARGET_FILE:${TARGET_NAME}>"
        ${CLIENT_PLUGIN_FULL_PATH}
    )

    # create the destination for the server plugin binaries
    add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
        ${SERVER_PLUGIN_FULL_PATH}
    )
    # copy the server plugin binaries
    add_custom_command(TARGET ${DIR} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy
        "$<TARGET_FILE:${TARGET_NAME}>"
        ${SERVER_PLUGIN_FULL_PATH}
    )

endmacro()
