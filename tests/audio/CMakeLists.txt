set(TARGET_NAME audio-tests)

setup_hifi_project()

# link in the shared libraries
link_hifi_libraries(shared audio networking)

include_dependency_includes()