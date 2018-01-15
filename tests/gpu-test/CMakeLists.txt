set(TARGET_NAME gpu-test)
AUTOSCRIBE_SHADER_LIB(gpu model render-utils)
# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Quick Gui OpenGL Script Widgets)
setup_memory_debugger()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")
link_hifi_libraries(
    shared networking gl 
    ktx gpu procedural octree image 
    model model-networking fbx animation 
    script-engine render render-utils 
    ${PLATFORM_GL_BACKEND}
)

if (WIN32)
  add_dependency_external_projects(wasapi)
endif ()

package_libraries_for_deployment()

target_nsight()
