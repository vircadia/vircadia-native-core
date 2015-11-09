set(EXTERNAL_NAME boostconfig)
string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

include(ExternalProject)
ExternalProject_Add(
    ${EXTERNAL_NAME}
    #URL https://github.com/boostorg/config/archive/boost-1.58.0.zip
    URL http://hifi-public.s3.amazonaws.com/dependencies/config-boost-1.58.0.zip
    URL_MD5 42fa673bae2b7645a22736445e80eb8d
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
)

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE TYPE INTERNAL)

set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
