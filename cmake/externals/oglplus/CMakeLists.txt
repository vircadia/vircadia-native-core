set(EXTERNAL_NAME oglplus)
string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

include(ExternalProject)
ExternalProject_Add(
    ${EXTERNAL_NAME}
    GIT_REPOSITORY https://github.com/jherico/oglplus.git
    GIT_TAG 470d8e56fd6bf3913ceec03d82f42d3bafab2cbe
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
)

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include ${SOURCE_DIR}/implement CACHE TYPE INTERNAL)

