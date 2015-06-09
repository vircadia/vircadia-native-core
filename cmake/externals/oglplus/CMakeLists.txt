set(EXTERNAL_NAME oglplus)
string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

include(ExternalProject)
ExternalProject_Add(
    ${EXTERNAL_NAME}
    GIT_REPOSITORY https://github.com/jherico/oglplus.git
    GIT_TAG e67b24a6e3397c2dc16523bcd5d4c229abd20b81
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
)

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include ${SOURCE_DIR}/implement CACHE TYPE INTERNAL)

