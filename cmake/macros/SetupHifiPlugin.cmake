#
#  Created by Bradley Austin Davis on 2015/10/25
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#
macro(SETUP_HIFI_PLUGIN)
    set(${TARGET_NAME}_SHARED 1)
    setup_hifi_library(${ARGV})
    if (BUILD_CLIENT)
        add_dependencies(interface ${TARGET_NAME})
    endif()
    target_link_libraries(${TARGET_NAME} ${CMAKE_THREAD_LIBS_INIT})
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Plugins")

    if (APPLE)
        set(PLUGIN_PATH "${INTERFACE_BUNDLE_NAME}.app/Contents/PlugIns")
    else()
        set(PLUGIN_PATH "plugins")
    endif()

    if (WIN32)
        # produce PDB files for plugins as well
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
    endif()

    if (CMAKE_SYSTEM_NAME MATCHES "Linux" OR CMAKE_GENERATOR STREQUAL "Unix Makefiles")
        set(PLUGIN_FULL_PATH "${CMAKE_BINARY_DIR}/interface/${PLUGIN_PATH}/")
    else()
        set(PLUGIN_FULL_PATH "${CMAKE_BINARY_DIR}/interface/$<CONFIGURATION>/${PLUGIN_PATH}/")
    endif()

    # create the destination for the plugin binaries
    add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
        ${PLUGIN_FULL_PATH}
    )

    add_custom_command(TARGET ${DIR} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy
        "$<TARGET_FILE:${TARGET_NAME}>"
        ${PLUGIN_FULL_PATH}
    )
endmacro()
