set(TARGET_NAME physics-tests)

setup_hifi_project()

include_glm()
include_bullet()

link_hifi_libraries(shared physics)

link_shared_dependencies()
