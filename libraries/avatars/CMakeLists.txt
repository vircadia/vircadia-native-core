set(TARGET_NAME avatars)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Network Script)

add_dependency_external_project(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})

link_hifi_libraries(audio shared networking)

# call macro to include our dependency includes and bubble them up via a property on our target
include_dependency_includes()
