set(TARGET_NAME shared-tests)

setup_hifi_project()

include_glm()

# link in the shared libraries
link_hifi_libraries(shared)

include_dependency_includes()