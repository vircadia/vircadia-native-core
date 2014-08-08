set(TARGET_NAME physics-tests)

setup_hifi_project()

include_glm()

# link in the shared libraries
link_hifi_libraries(shared)

link_shared_dependencies()