set(EXTERNAL_NAME GifCreator)

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL "${EXTERNAL_BUILD_ASSETS}/dependencies/GifCreator.zip"
  URL_MD5 8ac8ef5196f47c658dce784df5ecdb70
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR}/src/${EXTERNAL_NAME} CACHE PATH "List of GifCreator include directories")
