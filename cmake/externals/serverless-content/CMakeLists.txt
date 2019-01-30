include(ExternalProject)

set(EXTERNAL_NAME serverless-content)

ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL http://cdn.highfidelity.com/content-sets/serverless-tutorial-RC75.zip
  URL_MD5 b4225d058952e17976ac228330ce8d51
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD 1
)

# Hide this external target (for IDE users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
