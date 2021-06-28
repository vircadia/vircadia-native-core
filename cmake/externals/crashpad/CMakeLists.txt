include(ExternalProject)
set(EXTERNAL_NAME crashpad)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (WIN32)
    ExternalProject_Add(
      ${EXTERNAL_NAME}
      URL "${EXTERNAL_BUILD_ASSETS}/dependencies/crashpad_linux_20210628_cdee8004.tar.bz2"
      URL_SHA256 2e84b253b27f37224f5f9f86f955b23befcf00cd3300e66de42a863c82ec2787
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
      LOG_DOWNLOAD 1
    )

    ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

    set(BIN_RELEASE_PATH "${SOURCE_DIR}/out/Release_x64")
    set(BIN_EXT ".exe")
    set(LIB_RELEASE_PATH "${SOURCE_DIR}/out/Release_x64/lib_MD")
    set(LIB_DEBUG_PATH "${SOURCE_DIR}/out/Debug_x64/lib_MD")
    set(LIB_PREFIX "")
    set(LIB_EXT "lib")
elseif (APPLE)
  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL "${EXTERNAL_BUILD_ASSETS}/dependencies/crashpad_mac_070318.zip"
    URL_MD5 ba1501dc163591ac2d1be74946967e2a
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
  )

  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

  set(BIN_RELEASE_PATH "${SOURCE_DIR}/out/Release")
  set(BIN_EXT "")
  set(LIB_RELEASE_PATH "${SOURCE_DIR}/out/Release/lib")
  set(LIB_DEBUG_PATH "${SOURCE_DIR}/out/Debug/lib")
  set(LIB_PREFIX "lib")
  set(LIB_EXT "a")
elseif (UNIX)
  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL "${EXTERNAL_BUILD_ASSETS}/dependencies/crashpad_linux_f1943fcb.tar.bz2"
    URL_MD5 e0949e5988905471c63c399833879482
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
  )

  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

  set(BIN_RELEASE_PATH "${SOURCE_DIR}/out/Release")
  set(BIN_EXT "")
  set(LIB_RELEASE_PATH "${SOURCE_DIR}/out/Release/lib")
  set(LIB_DEBUG_PATH "${SOURCE_DIR}/out/Debug/lib")
  set(LIB_PREFIX "lib")
  set(LIB_EXT "a")
endif ()

if (WIN32 OR APPLE OR UNIX)
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE PATH "List of Crashpad include directories")

  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${LIB_RELEASE_PATH}/${LIB_PREFIX}crashpad_client.${LIB_EXT} CACHE FILEPATH "Path to Crashpad release library")
  set(${EXTERNAL_NAME_UPPER}_BASE_LIBRARY_RELEASE ${LIB_RELEASE_PATH}/${LIB_PREFIX}base.${LIB_EXT} CACHE FILEPATH "Path to Crashpad base release library")
  set(${EXTERNAL_NAME_UPPER}_UTIL_LIBRARY_RELEASE ${LIB_RELEASE_PATH}/${LIB_PREFIX}crashpad_util.${LIB_EXT} CACHE FILEPATH "Path to Crashpad util release library")

  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${LIB_DEBUG_PATH}/${LIB_PREFIX}crashpad_client.${LIB_EXT} CACHE FILEPATH "Path to Crashpad debug library")
  set(${EXTERNAL_NAME_UPPER}_BASE_LIBRARY_DEBUG ${LIB_DEBUG_PATH}/${LIB_PREFIX}base.${LIB_EXT} CACHE FILEPATH "Path to Crashpad base debug library")
  set(${EXTERNAL_NAME_UPPER}_UTIL_LIBRARY_DEBUG ${LIB_DEBUG_PATH}/${LIB_PREFIX}crashpad_util.${LIB_EXT} CACHE FILEPATH "Path to Crashpad util debug library")

  set(CRASHPAD_HANDLER_EXE_PATH ${BIN_RELEASE_PATH}/crashpad_handler${BIN_EXT} CACHE FILEPATH "Path to the Crashpad handler binary")
endif ()

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
