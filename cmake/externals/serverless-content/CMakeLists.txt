include(ExternalProject)

set(EXTERNAL_NAME serverless-content)

ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL http://cdn.highfidelity.com/content-sets/serverless-tutorial-RC67.zip
  URL_MD5 3fc4b7332be771d71b43b6d688de9aa7
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD 1
)

# Hide this external target (for IDE users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
