
set(TARGET_NAME render-texture-load)
 
if (WIN32)
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4049 /ignore:4217")
endif()

setup_memory_debugger()
setup_thread_debugger()

# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Quick Gui)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")

# link in the shared libraries
link_hifi_libraries(
    shared task networking octree 
    shaders gl gpu render ktx image animation 
    graphics hfm fbx model-networking 
    render-utils 
    entities entities-renderer audio avatars 
    script-engine physics 
    ${PLATFORM_GL_BACKEND}
)

package_libraries_for_deployment()

target_gli()
target_glm()

target_zlib()
target_quazip()

if (WIN32)
  add_dependency_external_projects(wasapi)
endif ()


target_bullet()
