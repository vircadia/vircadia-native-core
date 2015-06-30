set(TARGET_NAME input-plugins)

setup_hifi_library()

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
link_hifi_libraries(shared plugins)

GroupSources("src/input-plugins")

add_dependency_external_projects(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})

add_dependency_external_projects(OpenVR)
find_package(OpenVR REQUIRED)
target_include_directories(${TARGET_NAME} PRIVATE ${OPENVR_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${OPENVR_LIBRARIES})

add_dependency_external_projects(Sixense)
find_package(Sixense REQUIRED)
target_include_directories(${TARGET_NAME} PRIVATE ${SIXENSE_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${SIXENSE_LIBRARIES})
