set(TARGET_NAME gpu-test)
AUTOSCRIBE_SHADER_LIB(gpu graphics render-utils)
# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Quick Gui Script)
setup_memory_debugger()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")
link_hifi_libraries(
    shared networking gl 
    ktx gpu procedural octree image 
    graphics model-networking fbx animation 
    script-engine render render-utils 
    ${PLATFORM_GL_BACKEND}
)

if (WIN32)
  add_dependency_external_projects(wasapi)
endif ()

package_libraries_for_deployment()

target_nsight()
