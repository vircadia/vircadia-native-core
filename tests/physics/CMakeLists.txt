set(TARGET_NAME physics-tests)

setup_hifi_project()

add_dependency_external_project(glm)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})

include_bullet()

link_hifi_libraries(shared physics)

include_dependency_includes()
