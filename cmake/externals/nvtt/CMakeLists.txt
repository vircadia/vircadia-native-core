include(ExternalProject)
include(SelectLibraryConfigurations)

set(EXTERNAL_NAME nvtt)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (NOT ANDROID)
  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL "https://s3-us-west-1.amazonaws.com/hifi-content/clement/dev/nvtt.zip"
    URL_MD5 "3ea6eeadbcc69071acf9c49ba565760e"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
  )

  # Hide this external target (for IDE users)
  set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE PATH "Location of NVTT include directory")

  if (WIN32)
    set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/Release/x64/nvtt.lib CACHE FILEPATH "Path to NVTT library")
    set(${EXTERNAL_NAME_UPPER}_DLL_PATH ${SOURCE_DIR}/Release/x64 CACHE PATH "Location of NVTT DLL")
    add_paths_to_fixup_libs(${${EXTERNAL_NAME_UPPER}_DLL_PATH})
  else (APPLE)
    set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/static/libnvtt.a CACHE FILEPATH "Path to NVTT static library")
  elseif ()
    set(${EXTERNAL_NAME_UPPER}_LIBRARY_TEMP ${INSTALL_DIR}/lib/nvtt.so CACHE FILEPATH "Path to NVTT library")
  endif ()

endif()
