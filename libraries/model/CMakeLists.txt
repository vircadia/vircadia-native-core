set(TARGET_NAME model)

AUTOSCRIBE_SHADER_LIB(gpu)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library()

add_dependency_external_projects(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})

link_hifi_libraries(shared gpu octree)
