set(TARGET_NAME script-engine)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Gui Network Script Widgets)

add_dependency_external_projects(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})

link_hifi_libraries(shared octree gpu model fbx entities animation audio physics metavoxels)

# call macro to include our dependency includes and bubble them up via a property on our target
include_dependency_includes()

