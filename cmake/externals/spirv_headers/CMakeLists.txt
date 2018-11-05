set(EXTERNAL_NAME spirv_headers)
string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL https://github.com/KhronosGroup/SPIRV-Headers/archive/2c512180ca03b5d4f56283efc85745775b45fdc4.zip
  URL_MD5 83e652221b5f21d5fdb61c45f5b4d9f9
  CONFIGURE_COMMAND CMAKE_ARGS  ${ANDROID_CMAKE_ARGS} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> ${EXTRA_CMAKE_FLAGS}
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)
set(${EXTERNAL_NAME_UPPER}_ROOT ${INSTALL_DIR} CACHE PATH "List of  include directories")
