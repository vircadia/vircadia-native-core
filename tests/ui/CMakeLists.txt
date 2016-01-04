
set(TARGET_NAME "ui-test")

# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Network OpenGL Qml Quick Script WebChannel WebEngine WebSockets)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")

if (WIN32)
  target_link_libraries(${TARGET_NAME} wsock32.lib opengl32.lib Winmm.lib)
  # Issue causes build failure unless we add this directory.  
  # See https://bugreports.qt.io/browse/QTBUG-43351  
  add_paths_to_fixup_libs(${Qt5_DIR}/../../../plugins/qtwebengine)
endif()

# link in the shared libraries
link_hifi_libraries(shared networking gl gpu ui)

# copy the resources files beside the executable
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
  "${PROJECT_SOURCE_DIR}/qml"
  $<TARGET_FILE_DIR:${TARGET_NAME}>/qml
)


target_glew()

if (WIN32)
    set(EXTRA_DEPLOY_OPTIONS "--qmldir  ${PROJECT_SOURCE_DIR}/../../interface/resources/qml")
endif()

package_libraries_for_deployment()
