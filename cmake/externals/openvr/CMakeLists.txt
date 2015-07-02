include(ExternalProject)
include(SelectLibraryConfigurations)

set(EXTERNAL_NAME OpenVR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL https://github.com/ValveSoftware/openvr/archive/0.9.2.zip
  URL_MD5 3ff7b67861b63ef673014e353f2c1bf6
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD 1
)

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/headers CACHE TYPE INTERNAL)

if (WIN32)

  # FIXME need to account for different architectures 
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/win32/openvr_api.lib CACHE TYPE INTERNAL)
  add_paths_to_fixup_libs(${SOURCE_DIR}/bin/win32)

elseif(APPLE)

  # FIXME need to account for different architectures 
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/osx32/libopenvr_api.dylib CACHE TYPE INTERNAL)
  add_paths_to_fixup_libs(${SOURCE_DIR}/bin/osx32)

elseif(NOT ANDROID)

  # FIXME need to account for different architectures 
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/linux32/libopenvr_api.so CACHE TYPE INTERNAL)
  add_paths_to_fixup_libs(${SOURCE_DIR}/bin/linux32)
  
endif()

