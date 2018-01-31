include(ExternalProject)
set(EXTERNAL_NAME crashpad)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (WIN32)
    ExternalProject_Add(
      ${EXTERNAL_NAME}
      URL https://backtrace.io/download/crashpad_062317.zip
      URL_MD5 65817e564b3628492abfc1dbd2a1e98b
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
      LOG_DOWNLOAD 1
    )

    ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

    set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE PATH "List of Crashpad include directories")

    set(LIB_EXT "lib")

    set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${SOURCE_DIR}/out/Release_x64/lib_MD/${LIB_PREFIX}crashpad_client.${LIB_EXT} CACHE FILEPATH "Path to Crashpad release library")
    set(${EXTERNAL_NAME_UPPER}_BASE_LIBRARY_RELEASE ${SOURCE_DIR}/out/Release_x64/lib_MD/${LIB_PREFIX}base.${LIB_EXT} CACHE FILEPATH "Path to Crashpad base release library")
    set(${EXTERNAL_NAME_UPPER}_UTIL_LIBRARY_RELEASE ${SOURCE_DIR}/out/Release_x64/lib_MD/${LIB_PREFIX}crashpad_util.${LIB_EXT} CACHE FILEPATH "Path to Crashpad util release library")

    set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${SOURCE_DIR}/out/Debug_x64/lib_MD/${LIB_PREFIX}crashpad_client.${LIB_EXT} CACHE FILEPATH "Path to Crashpad debug library")
    set(${EXTERNAL_NAME_UPPER}_BASE_LIBRARY_DEBUG ${SOURCE_DIR}/out/Debug_x64/lib_MD/${LIB_PREFIX}base.${LIB_EXT} CACHE FILEPATH "Path to Crashpad base debug library")
    set(${EXTERNAL_NAME_UPPER}_UTIL_LIBRARY_DEBUG ${SOURCE_DIR}/out/Debug_x64/lib_MD/${LIB_PREFIX}crashpad_util.${LIB_EXT} CACHE FILEPATH "Path to Crashpad util debug library")

    set(CRASHPAD_HANDLER_EXE_PATH ${SOURCE_DIR}/out/Release_x64/crashpad_handler.exe CACHE FILEPATH "Path to the Crashpad handler executable")
endif ()

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
