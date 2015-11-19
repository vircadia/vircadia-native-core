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
    add_dependencies(interface ${TARGET_NAME})
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Plugins")
    
    if (APPLE) 
        set(PLUGIN_PATH "interface.app/Contents/MacOS/plugins")
    else()
        set(PLUGIN_PATH "plugins")
    endif()
    
    # create the destination for the plugin binaries
    add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${CMAKE_BINARY_DIR}/interface/$<CONFIGURATION>/${PLUGIN_PATH}/"
    )
    
    add_custom_command(TARGET ${DIR} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy
        "$<TARGET_FILE:${TARGET_NAME}>"
        "${CMAKE_BINARY_DIR}/interface/$<CONFIGURATION>/${PLUGIN_PATH}/"
    )

endmacro()