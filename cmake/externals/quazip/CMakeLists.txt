set(EXTERNAL_NAME quazip)

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL https://s3-us-west-1.amazonaws.com/hifi-production/dependencies/quazip-0.6.2.zip
  URL_MD5 514851970f1a14d815bdc3ad6267af4d
  BINARY_DIR ${EXTERNAL_PROJECT_PREFIX}/build
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DCMAKE_PREFIX_PATH=$ENV{QT_CMAKE_PREFIX_PATH}
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR}/include CACHE PATH "List of QuaZip include directories")

if (APPLE)
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${INSTALL_DIR}/lib/libquazip.1.0.0.dylib CACHE FILEPATH "List of QuaZip libraries")
endif ()
