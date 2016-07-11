include(ExternalProject)
include(SelectLibraryConfigurations)

set(EXTERNAL_NAME HiFiAudioCodec)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL https://s3.amazonaws.com/hifi-public/dependencies/codecSDK.zip
  URL_MD5 4add25b7cc5dfdb3cbfc1156b95cfff7
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE TYPE INTERNAL)

if (WIN32)
    set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/Release/audio.lib CACHE TYPE INTERNAL)

  # FIXME need to account for different architectures
  #if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
  #  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/win64/audio.lib CACHE TYPE INTERNAL)
  #  add_paths_to_fixup_libs(${SOURCE_DIR}/bin/win64)
  #else()
  #  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/win32/audio.lib CACHE TYPE INTERNAL)
  #  add_paths_to_fixup_libs(${SOURCE_DIR}/bin/win32)
  #endif()

elseif(APPLE)

  # FIXME need to account for different architectures 
  #set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/osx32/audio.dylib CACHE TYPE INTERNAL)
  #add_paths_to_fixup_libs(${SOURCE_DIR}/bin/osx32)

elseif(NOT ANDROID)

  # FIXME need to account for different architectures 
  #set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/linux64/audio.so CACHE TYPE INTERNAL)
  #add_paths_to_fixup_libs(${SOURCE_DIR}/bin/linux64)
  
endif()

