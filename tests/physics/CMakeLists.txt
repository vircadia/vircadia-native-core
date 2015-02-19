set(TARGET_NAME physics-tests)

setup_hifi_project()

add_dependency_external_projects(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})

add_dependency_external_projects(bullet)

find_package(Bullet REQUIRED)
target_include_directories(${TARGET_NAME} PRIVATE SYSTEM ${BULLET_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${BULLET_LIBRARIES})

link_hifi_libraries(shared physics)

copy_dlls_beside_windows_executable()