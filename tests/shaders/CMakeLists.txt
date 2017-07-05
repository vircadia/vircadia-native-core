
set(TARGET_NAME shaders-test)
 
# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Quick Gui OpenGL)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")

setup_memory_debugger()

# link in the shared libraries
link_hifi_libraries(shared octree gl gpu gpu-gl model render fbx networking entities 
                    script-engine physics 
                    render-utils entities-renderer)

include_directories("${PROJECT_BINARY_DIR}/../../libraries/gpu/")
include_directories("${PROJECT_BINARY_DIR}/../../libraries/render-utils/")
include_directories("${PROJECT_BINARY_DIR}/../../libraries/entities-renderer/")
include_directories("${PROJECT_BINARY_DIR}/../../libraries/model/")

package_libraries_for_deployment()
