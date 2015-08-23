
set(TARGET_NAME "ui-test")

# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Widgets OpenGL Network Qml Quick Script)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")

if (WIN32)
  target_link_libraries(${TARGET_NAME} wsock32.lib opengl32.lib Winmm.lib)
endif()

# link in the shared libraries
link_hifi_libraries(ui render-utils gpu shared)

setup_memory_debugger()

copy_dlls_beside_windows_executable()
