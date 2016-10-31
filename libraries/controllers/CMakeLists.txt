set(TARGET_NAME controllers)

# set a default root dir for each of our optional externals if it was not passed
setup_hifi_library(Script Qml)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
link_hifi_libraries(shared)

GroupSources("src/controllers")

add_dependency_external_projects(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})
