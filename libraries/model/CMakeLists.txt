set(TARGET_NAME model)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library()

add_dependency_external_project(glm)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})

link_hifi_libraries(shared gpu)

# call macro to link our dependencies and bubble them up via a property on our target
include_dependency_includes()
