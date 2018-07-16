set(EXTERNAL_NAME glm)

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL https://public.highfidelity.com/dependencies/glm-0.9.8.5-patched.zip
  URL_MD5 7d39ecc1cea275427534c3cfd6dd63f0
  BINARY_DIR ${EXTERNAL_PROJECT_PREFIX}/build
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> ${EXTERNAL_ARGS}
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR}/include CACHE PATH "List of glm include directories")
