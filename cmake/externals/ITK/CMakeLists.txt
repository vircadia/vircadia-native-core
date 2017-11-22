set(EXTERNAL_NAME ITK)

include(ExternalProject)
ExternalProject_Add(
    ${EXTERNAL_NAME}
    PREFIX ${EXTERNAL_NAME}
    GIT_REPOSITORY https://github.com/InsightSoftwareConsortium/ITK.git
    GIT_TAG d7ab07bdaee74b45e890aa3070de934eef381c3e
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

set(${EXTERNAL_NAME}_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE TYPE INTERNAL)

set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
