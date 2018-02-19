
set(TARGET_NAME render-utils-test)
 
# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Quick Gui)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")

setup_memory_debugger()

# link in the shared libraries
link_hifi_libraries(render-utils gl gpu shared ${PLATFORM_GL_BACKEND})
target_link_libraries(${TARGET_NAME} ${CMAKE_THREAD_LIBS_INIT})

if (WIN32)
  add_dependency_external_projects(wasapi)
endif ()

package_libraries_for_deployment()
