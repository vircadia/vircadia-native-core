set(TARGET_NAME domain-server)

# setup the project and link required Qt modules
setup_hifi_project(${TARGET_NAME} Network)

# remove and then copy the files for the webserver
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                   COMMAND "${CMAKE_COMMAND}" -E remove_directory
                   $<TARGET_FILE_DIR:${TARGET_NAME}>/resources/web)
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                  COMMAND "${CMAKE_COMMAND}" -E copy_directory
                  "${PROJECT_SOURCE_DIR}/resources/web"
                  $<TARGET_FILE_DIR:${TARGET_NAME}>/resources/web)

# link the shared hifi libraries
link_hifi_libraries(${TARGET_NAME} embedded-webserver networking shared)

# link any shared dependencies
link_shared_dependencies_to_target(${TARGET_NAME})