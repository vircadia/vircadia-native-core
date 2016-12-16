include(ExternalProject)
include(SelectLibraryConfigurations)

set(EXTERNAL_NAME LibOVRPlatform)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (WIN32)

  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL http://hifi-public.s3.amazonaws.com/dependencies/OVRPlatformSDK_v1.10.0.zip
    URL_MD5 e6c8264af16d904e6506acd5172fa0a9
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
  )

  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${SOURCE_DIR}/Windows/LibOVRPlatform64_1.lib CACHE TYPE INTERNAL)
  else()
    set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${SOURCE_DIR}/Windows/LibOVRPlatform32_1.lib CACHE TYPE INTERNAL)
  endif()

  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/Include CACHE TYPE INTERNAL)
endif ()

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
