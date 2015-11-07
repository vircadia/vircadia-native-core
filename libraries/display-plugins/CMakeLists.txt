set(TARGET_NAME display-plugins)
setup_hifi_library(OpenGL)
link_hifi_libraries(shared plugins gl)

target_opengl()

GroupSources("src/display-plugins")

target_oglplus()

if (WIN32)
  add_dependency_external_projects(OpenVR)
  find_package(OpenVR REQUIRED)
  target_include_directories(${TARGET_NAME} PRIVATE ${OPENVR_INCLUDE_DIRS})
  target_link_libraries(${TARGET_NAME} ${OPENVR_LIBRARIES})
endif()
