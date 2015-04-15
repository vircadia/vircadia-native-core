set(TARGET_NAME vhacd-util)
setup_hifi_project(Core Widgets)
link_hifi_libraries(shared fbx model gpu)

add_dependency_external_projects(vhacd)

find_package(VHACD REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${VHACD_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${VHACD_LIBRARIES})
