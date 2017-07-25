set(EXTERNAL_NAME eigen)

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL http://bitbucket.org/eigen/eigen/get/3.3.4.zip
  URL_MD5 e337acc279874bc6a56da4d973a723fb
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR} CACHE PATH "List of eigen include directories")