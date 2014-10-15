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

# find OpenSSL
find_package(OpenSSL REQUIRED)

if (APPLE AND ${OPENSSL_INCLUDE_DIR} STREQUAL "/usr/include")
  # this is a user on OS X using system OpenSSL, which is going to throw warnings since they're deprecating for their common crypto
  message(WARNING "The found version of OpenSSL is the OS X system version. This will produce deprecation warnings."
  "\nWe recommend you install a newer version (at least 1.0.1h) in a different directory and set OPENSSL_ROOT_DIR in your env so Cmake can find it.")
endif ()

include_directories(SYSTEM "${OPENSSL_INCLUDE_DIR}")

# append OpenSSL to our list of libraries to link
list(APPEND ${TARGET_NAME}_LIBRARIES_TO_LINK "${OPENSSL_LIBRARIES}")

link_shared_dependencies()