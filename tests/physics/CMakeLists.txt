set(TARGET_NAME physics-tests)

setup_hifi_project()

add_dependency_external_projects(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})

include_bullet()

link_hifi_libraries(shared physics)

copy_dlls_beside_windows_executable()