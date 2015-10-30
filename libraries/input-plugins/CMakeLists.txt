set(TARGET_NAME input-plugins)
setup_hifi_library()
link_hifi_libraries(shared plugins controllers render-utils)

GroupSources("src/input-plugins")

if (WIN32)
    add_dependency_external_projects(OpenVR)
    find_package(OpenVR REQUIRED)
    target_include_directories(${TARGET_NAME} PRIVATE ${OPENVR_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${OPENVR_LIBRARIES})
endif()

target_sdl2()
target_sixense()
