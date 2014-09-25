set(TARGET_NAME domain-server)

# setup the project and link required Qt modules
setup_hifi_project(Network)

# remove and then copy the files for the webserver
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                   COMMAND "${CMAKE_COMMAND}" -E remove_directory
                   $<TARGET_FILE_DIR:${TARGET_NAME}>/resources)
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                  COMMAND "${CMAKE_COMMAND}" -E copy_directory
                  "${PROJECT_SOURCE_DIR}/resources"
                  $<TARGET_FILE_DIR:${TARGET_NAME}>/resources)

# link the shared hifi libraries
link_hifi_libraries(embedded-webserver networking shared)

link_shared_dependencies()