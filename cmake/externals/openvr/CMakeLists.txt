include(ExternalProject)
include(SelectLibraryConfigurations)

set(EXTERNAL_NAME OpenVR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL https://github.com/ValveSoftware/openvr/archive/v1.0.2.zip
  URL_MD5 0d1cf5f579cf092e33f34759967b7046
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/headers CACHE TYPE INTERNAL)

if (WIN32)

  # FIXME need to account for different architectures
  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/win64/openvr_api.lib CACHE TYPE INTERNAL)
    add_paths_to_fixup_libs(${SOURCE_DIR}/bin/win64)
  else()
    set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/win32/openvr_api.lib CACHE TYPE INTERNAL)
    add_paths_to_fixup_libs(${SOURCE_DIR}/bin/win32)
  endif()

elseif(APPLE)

  # FIXME need to account for different architectures 
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/osx32/libopenvr_api.dylib CACHE TYPE INTERNAL)
  add_paths_to_fixup_libs(${SOURCE_DIR}/bin/osx32)

elseif(NOT ANDROID)

  # FIXME need to account for different architectures 
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/linux64/libopenvr_api.so CACHE TYPE INTERNAL)
  add_paths_to_fixup_libs(${SOURCE_DIR}/bin/linux64)
  
endif()

