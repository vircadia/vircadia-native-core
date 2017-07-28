
set(TARGET_NAME render-perf-test)
 
if (WIN32)
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4049 /ignore:4217")
endif()

setup_memory_debugger()

# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Quick Gui OpenGL)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")

# link in the shared libraries
link_hifi_libraries(shared networking model fbx ktx image octree gl gpu gpu-gl render model-networking networking render-utils entities entities-renderer animation audio avatars script-engine physics procedural midi)

package_libraries_for_deployment()


target_bullet()
