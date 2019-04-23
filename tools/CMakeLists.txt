# add the tool directories
find_npm()

if (NPM_EXECUTABLE)
  add_subdirectory(jsdoc)
  set_target_properties(jsdoc PROPERTIES FOLDER "Tools")
endif()

function(check_test name)
    set(RESULT TRUE)
    if (BUILD_TOOLS_INCLUDE)
        unset(RESULT)
        list(FIND BUILD_TOOLS_INCLUDE ${name} BUILD_TOOL_FIND)
        if (NOT (${BUILD_TOOL_FIND} EQUAL -1))
            set(RESULT TRUE)
        endif()
    endif()
    set(BUILD_TOOL_RESULT ${RESULT} PARENT_SCOPE)
endfunction()

if (BUILD_TOOLS)
    # Allow different tools for stable builds
    if (STABLE_BUILD)
        set(ALL_TOOLS
            udt-test
            vhacd-util
            frame-optimizer
            gpu-frame-player
            ice-client
            ktx-tool
            ac-client
            skeleton-dump
            atp-client
            # oven
        )
    else()
        set(ALL_TOOLS
            udt-test
            vhacd-util
            frame-optimizer
            gpu-frame-player
            ice-client
            ktx-tool
            ac-client
            skeleton-dump
            atp-client
            # oven
            nitpick
        )
    endif()

    foreach(TOOL ${ALL_TOOLS})
        check_test(${TOOL})
        if (${BUILD_TOOL_RESULT})
            add_subdirectory(${TOOL})
            set_target_properties(${TOOL} PROPERTIES FOLDER "Tools")
        endif()
    endforeach()
endif()
