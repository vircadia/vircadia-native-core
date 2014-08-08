set(TARGET_NAME networking-tests)

setup_hifi_project(${TARGET_NAME})

# link in the shared libraries
link_hifi_libraries(${TARGET_NAME} shared networking)

