set(EXTERNAL_NAME zlib)

include(ExternalProject)

if (WIN32)
  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL http://zlib.net/zlib128-dll.zip
    URL_MD5 42eccc2af5bac6b7a1188d7817d03549
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
  )
  
  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)
  
  set(${EXTERNAL_NAME_UPPER}_LIBRARY ${SOURCE_DIR}/lib/zdll.lib CACHE TYPE "Location of zlib library")
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIR ${SOURCE_DIR}/include CACHE TYPE "Location of zlib includes")
  set(${EXTERNAL_NAME_UPPER}_DLL_PATH ${SOURCE_DIR} CACHE TYPE "Location of zlib DLL")
endif ()

