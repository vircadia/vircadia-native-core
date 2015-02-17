set(EXTERNAL_NAME soxr)

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  PREFIX ${EXTERNAL_NAME}
  URL http://pkgs.fedoraproject.org/repo/pkgs/soxr/soxr-0.1.1-Source.tar.xz/805651a245ead381c82fad6247af5968/soxr-0.1.1-Source.tar.xz
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DBUILD_SHARED_LIBS=N
  LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR}/include CACHE TYPE STRING)

if (WIN32) 
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${INSTALL_DIR}/lib/libsoxr.lib CACHE TYPE STRING)
else ()
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${INSTALL_DIR}/lib/libsoxr.a CACHE TYPE STRING)
endif ()