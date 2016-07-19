include(ExternalProject)
include(SelectLibraryConfigurations)

set(EXTERNAL_NAME hifiAudioCodec)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (WIN32 OR APPLE)
  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL http://s3.amazonaws.com/hifi-public/dependencies/codecSDK-1.zip
    URL_MD5 23ec3fe51eaa155ea159a4971856fc13
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
  )
elseif(NOT ANDROID)
  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL http://s3.amazonaws.com/hifi-public/dependencies/codecSDK-linux.zip
    URL_MD5 7d37914a18aa4de971d2f45dd3043bde
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
  )
endif()

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE TYPE INTERNAL)

if (WIN32)
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/Release/audio.lib CACHE TYPE INTERNAL)
elseif(APPLE)
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/Release/libaudio.a CACHE TYPE INTERNAL)
elseif(NOT ANDROID)
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/Release/libaudio.a CACHE TYPE INTERNAL)
endif()
