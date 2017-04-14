include(ExternalProject)
include(SelectLibraryConfigurations)

set(EXTERNAL_NAME nvtt)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (NOT ANDROID)
  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL http://hifi-public.s3.amazonaws.com/dependencies/nvidia-texture-tools-2.1.0.zip
    URL_MD5 79842ea185c8c95aaa70cee91e77f032
    CONFIGURE_COMMAND CMAKE_ARGS  ${ANDROID_CMAKE_ARGS} -DNVTT_SHARED=0 -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    LOG_DOWNLOAD 1
    LOG_CONFIGURE 1
    LOG_BUILD 1
  )

  # Hide this external target (for IDE users)
  set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

  ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR}/include CACHE PATH "Location of NVTT include directory")

  if (WIN32)
    set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${INSTALL_DIR}/Debug/x64/nvtt.lib CACHE FILEPATH "Path to NVTT Debug library")
    set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/Release/x64/nvtt.lib CACHE FILEPATH "Path to NVTT Release library")

    set(${EXTERNAL_NAME_UPPER}_DLL_PATH ${INSTALL_DIR}/Release/x64 CACHE PATH "Location of NVTT DLL")
    add_paths_to_fixup_libs(${${EXTERNAL_NAME_UPPER}_DLL_PATH})
  else ()
    set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/static/libnvtt.a CACHE FILEPATH "Path to NVTT static library")
  endif ()

endif()
