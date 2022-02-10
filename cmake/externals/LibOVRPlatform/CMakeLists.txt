include(ExternalProject)
include(SelectLibraryConfigurations)

set(EXTERNAL_NAME LibOVRPlatform)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (WIN32)

  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL ${EXTERNAL_LIBOVR_PLATFORM_URLS}
    URL_MD5 ${EXTERNAL_LIBOVR_PLATFORM_MD5}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
  )

  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${SOURCE_DIR}/Windows/LibOVRPlatform64_1.lib CACHE STRING INTERNAL)
  else()
    set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${SOURCE_DIR}/Windows/LibOVRPlatform32_1.lib CACHE STRING INTERNAL)
  endif()

  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/Include CACHE STRING INTERNAL)
endif ()

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
