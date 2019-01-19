# add the tool directories
find_npm()

if (NPM_EXECUTABLE)
    add_subdirectory(jsdoc)
    set_target_properties(jsdoc PROPERTIES FOLDER "Tools")
endif()

if (BUILD_TOOLS)
    add_subdirectory(udt-test)
    set_target_properties(udt-test PROPERTIES FOLDER "Tools")

    add_subdirectory(vhacd-util)
    set_target_properties(vhacd-util PROPERTIES FOLDER "Tools")

    add_subdirectory(ice-client)
    set_target_properties(ice-client PROPERTIES FOLDER "Tools")

    add_subdirectory(ktx-tool)
    set_target_properties(ktx-tool PROPERTIES FOLDER "Tools")

    add_subdirectory(ac-client)
    set_target_properties(ac-client PROPERTIES FOLDER "Tools")

    add_subdirectory(skeleton-dump)
    set_target_properties(skeleton-dump PROPERTIES FOLDER "Tools")

    add_subdirectory(atp-client)
    set_target_properties(atp-client PROPERTIES FOLDER "Tools")

    add_subdirectory(oven)
    set_target_properties(oven PROPERTIES FOLDER "Tools")

    # No need to build on Android or in a Production build
    ##if (NOT ANDROID AND NOT RELEASE_TYPE STREQUAL "PRODUCTION")
        add_subdirectory(nitpick)
        set_target_properties(nitpick PROPERTIES FOLDER "Tools")
    ##endif()
endif()
