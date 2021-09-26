include(ExternalProject)

set(EXTERNAL_NAME steamworks)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

set(STEAMWORKS_URL "${EXTERNAL_BUILD_ASSETS}/dependencies/steamworks_sdk_137.zip")
set(STEAMWORKS_URL_MD5 "95ba9d0e3ddc04f8a8be17d2da806cbb")

ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL ${STEAMWORKS_URL}
  URL_MD5 ${STEAMWORKS_URL_MD5}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD 1
)

set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/public CACHE STRING INTERNAL)

if (WIN32)

    if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        set(ARCH_DIR ${SOURCE_DIR}/redistributable_bin/win64)
        set(ARCH_SUFFIX "64")
    else()
        set(ARCH_DIR ${SOURCE_DIR}/redistributable_bin)
        set(ARCH_SUFFIX "")
    endif()

    set(${EXTERNAL_NAME_UPPER}_DLL_PATH ${ARCH_DIR})
    set(${EXTERNAL_NAME_UPPER}_LIB_PATH ${ARCH_DIR})

    set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE "${${EXTERNAL_NAME_UPPER}_LIB_PATH}/steam_api${ARCH_SUFFIX}.lib" CACHE STRING INTERNAL)
    add_paths_to_fixup_libs("${${EXTERNAL_NAME_UPPER}_DLL_PATH}")

elseif(APPLE)

    set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${SOURCE_DIR}/redistributable_bin/osx32/libsteam_api.dylib CACHE STRING INTERNAL)

    set(_STEAMWORKS_LIB_DIR "${SOURCE_DIR}/redistributable_bin/osx32")
    ExternalProject_Add_Step(
      ${EXTERNAL_NAME}
      change-install-name
      COMMENT "Calling install_name_tool on libraries to fix install name for dylib linking"
      COMMAND ${CMAKE_COMMAND} -DINSTALL_NAME_LIBRARY_DIR=${_STEAMWORKS_LIB_DIR} -P ${EXTERNAL_PROJECT_DIR}/OSXInstallNameChange.cmake
      DEPENDEES install
      WORKING_DIRECTORY <SOURCE_DIR>
      LOG 1
    )

elseif(NOT ANDROID AND NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")

    # FIXME need to account for different architectures
    set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${SOURCE_DIR}/redistributable_bin/linux64/libsteam_api.so CACHE STRING INTERNAL)

endif()
