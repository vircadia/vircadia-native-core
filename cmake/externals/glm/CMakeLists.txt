set(EXTERNAL_NAME glm)

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  PREFIX ${EXTERNAL_NAME}
  URL http://pkgs.fedoraproject.org/repo/pkgs/glm/glm-0.9.5.4.zip/fab76fc982b256b46208e5c750ed456a/glm-0.9.5.4.zip
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR}/include CACHE TYPE "List of glm include directories")