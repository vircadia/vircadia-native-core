set(TARGET_NAME script-engine)

setup_memory_debugger()

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Gui Network Script Widgets)

add_dependency_external_projects(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})

link_hifi_libraries(shared octree gpu model fbx entities animation audio physics)
