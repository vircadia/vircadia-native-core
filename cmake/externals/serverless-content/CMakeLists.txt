include(ExternalProject)

set(EXTERNAL_NAME serverless-content)

ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL http://cdn.highfidelity.com/content-sets/serverless-tutorial-RC70v2.zip
  URL_MD5 35fcc8e635e71d0b00a08455a2582448
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD 1
)

# Hide this external target (for IDE users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
