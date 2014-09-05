set(TARGET_NAME networking-tests)

setup_hifi_project()

# link in the shared libraries
link_hifi_libraries(shared networking)

link_shared_dependencies()