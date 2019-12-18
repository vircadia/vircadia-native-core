include(ExternalProject)
include(SelectLibraryConfigurations)

set(EXTERNAL_NAME hifiAudioCodec)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (WIN32)
  set(DOWNLOAD_URL "${EXTERNAL_BUILD_ASSETS}/dependencies/codecSDK-win-2.0.zip")
  set(DOWNLOAD_MD5 9199d4dbd6b16bed736b235efe980e67)
elseif (APPLE)
  set(DOWNLOAD_URL "${EXTERNAL_BUILD_ASSETS}/dependencies/codecSDK-mac-2.0.zip")
  set(DOWNLOAD_MD5 21649881e7d5dc94f922179be96f76ba)
elseif (ANDROID)
  set(DOWNLOAD_URL "${EXTERNAL_BUILD_ASSETS}/dependencies/codecSDK-android-2.0.zip")
  set(DOWNLOAD_MD5 aef2a852600d498d58aa586668191683)
elseif (UNIX)
  set(DOWNLOAD_URL "${EXTERNAL_BUILD_ASSETS}/dependencies/codecSDK-linux-2.0.zip")
  set(DOWNLOAD_MD5 67fb7755f9bcafb98a9fceea53bc7481)
else()
  return()
endif()

ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL ${DOWNLOAD_URL}
  URL_MD5 ${DOWNLOAD_MD5}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE STRING INTERNAL)

if (WIN32)
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/Release/audio.lib CACHE STRING INTERNAL)
else()
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/Release/libaudio.a CACHE STRING INTERNAL)
endif()
