set(TARGET_NAME plugins)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(OpenGL)

link_hifi_libraries(shared)

add_dependency_external_projects(glm)
find_package(GLM REQUIRED)

setup_memory_debugger()

target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})
