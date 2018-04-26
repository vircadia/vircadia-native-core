
set(TARGET_NAME shaders-test)
 
# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Quick Gui)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")

setup_memory_debugger()

# link in the shared libraries
link_hifi_libraries(
    shared octree gl gpu graphics render fbx networking entities 
    script-engine physics 
    render-utils entities-renderer
    ${PLATFORM_GL_BACKEND}
)

include_directories("${PROJECT_BINARY_DIR}/../../libraries/gpu/")
include_directories("${PROJECT_BINARY_DIR}/../../libraries/render-utils/")
include_directories("${PROJECT_BINARY_DIR}/../../libraries/entities-renderer/")
include_directories("${PROJECT_BINARY_DIR}/../../libraries/graphics/")

if (WIN32)
  add_dependency_external_projects(wasapi)
endif ()

package_libraries_for_deployment()
