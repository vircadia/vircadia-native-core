
set(TARGET_NAME render-perf-test)
 
if (WIN32)
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4049 /ignore:4217")
endif()

setup_memory_debugger()

# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Quick Gui)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")

# link in the shared libraries
link_hifi_libraries(
    shared networking animation 
    ktx image octree gl gpu  
    render render-utils 
    graphics fbx model-networking 
    entities entities-renderer audio avatars script-engine 
    physics procedural midi ui
    ${PLATFORM_GL_BACKEND}
)

if (WIN32)
    target_link_libraries(${TARGET_NAME} Winmm.lib)
    add_dependency_external_projects(wasapi)
endif()

package_libraries_for_deployment()


target_bullet()
