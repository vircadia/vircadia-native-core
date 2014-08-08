set(TARGET_NAME jitter-tests)

setup_hifi_project(${TARGET_NAME} TRUE)

# link in the shared libraries
link_hifi_libraries(${TARGET_NAME} shared networking)