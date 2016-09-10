set(EXTERNAL_NAME gli)

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL https://hifi-public.s3.amazonaws.com/dependencies/gli-0.8.1.0.zip
  URL_MD5 00c990f59c12bbf367956ef399d6f798
  BINARY_DIR ${EXTERNAL_PROJECT_PREFIX}/build
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR} CACHE PATH "List of gli include directories")