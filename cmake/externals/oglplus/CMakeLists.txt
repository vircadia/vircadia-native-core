set(EXTERNAL_NAME oglplus)
string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

include(ExternalProject)
ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL http://iweb.dl.sourceforge.net/project/oglplus/oglplus-0.63.x/oglplus-0.63.0.zip
    URL_MD5 de984ab245b185b45c87415c0e052135
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include ${SOURCE_DIR}/implement CACHE TYPE INTERNAL)

