set(TARGET_NAME jitter-tests)

setup_hifi_project(${TARGET_NAME})

# link in the shared libraries
link_hifi_libraries(${TARGET_NAME} shared networking)

# link any shared dependencies
link_shared_dependencies_to_target(${TARGET_NAME})