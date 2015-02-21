set(TARGET_NAME vhacd)
setup_hifi_project()
link_hifi_libraries(shared model fbx gpu networking octree)

find_package(VHACD REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${VHACD_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${VHACD_LIBRARIES})

add_dependency_external_project(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})

include_dependency_includes()