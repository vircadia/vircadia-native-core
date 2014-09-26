set(TARGET_NAME domain-server)

# setup the project and link required Qt modules
setup_hifi_project(Network)

# remove current resources dir
add_custom_command(
  TARGET ${TARGET_NAME} POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E remove_directory
  $<TARGET_FILE_DIR:${TARGET_NAME}>/resources
)
# copy all files in resources, including web
add_custom_command(
  TARGET ${TARGET_NAME} POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
  "${PROJECT_SOURCE_DIR}/resources"
  $<TARGET_FILE_DIR:${TARGET_NAME}>/resources
)

if (NOT WIN32)
  # remove the web directory so we can make it a symlink
  add_custom_command(
    TARGET ${TARGET_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E remove_directory
    $<TARGET_FILE_DIR:${TARGET_NAME}>/resources/web
  )
  
  # make the web directory a symlink
  add_custom_command(
    TARGET ${TARGET_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E create_symlink
    "${PROJECT_SOURCE_DIR}/resources/web"
    $<TARGET_FILE_DIR:${TARGET_NAME}>/resources/web
  )
  
endif ()

# link the shared hifi libraries
link_hifi_libraries(embedded-webserver networking shared)

link_shared_dependencies()