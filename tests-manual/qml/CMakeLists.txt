set(TARGET_NAME qml-test)
setup_hifi_project(Quick Qml Gui OpenGL)
setup_memory_debugger()
setup_thread_debugger()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")
link_hifi_libraries(shared networking gl qml)

if (WIN32)
    set(EXTRA_DEPLOY_OPTIONS "--qmldir  \"${PROJECT_SOURCE_DIR}/qml\"")

    package_libraries_for_deployment()
endif()

target_nsight()
