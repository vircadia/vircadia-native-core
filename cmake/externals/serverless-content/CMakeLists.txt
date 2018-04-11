include(ExternalProject)

set(EXTERNAL_NAME serverless-content)

ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL http://cdn.highfidelity.com/content-sets/serverless-tutorial-RC66-v3.zip
  URL_MD5 ea608c2e4c90539ab3c1d66acf0e005a
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD 1
)

# Hide this external target (for IDE users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
