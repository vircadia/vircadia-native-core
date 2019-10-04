
if (WIN32)
    set(TARGET_NAME gpu-frame-converter)
    setup_memory_debugger()
    setup_hifi_project()
    set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 17)
    link_hifi_libraries( shared gpu shaders )
    package_libraries_for_deployment()
endif()