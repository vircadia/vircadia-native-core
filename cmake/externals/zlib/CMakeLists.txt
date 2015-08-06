
if (WIN32)
  set(EXTERNAL_NAME zlib)
  string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

  include(ExternalProject)
  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL http://zlib.net/zlib128.zip
    URL_MD5 126f8676442ffbd97884eb4d6f32afb4
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
  )

  # Hide this external target (for ide users)
  set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE PATH "List of zlib include directories")

  ExternalProject_Get_Property(${EXTERNAL_NAME} BINARY_DIR)

  set(${EXTERNAL_NAME_UPPER}_DLL_PATH ${BINARY_DIR}/Release CACHE FILEPATH "Location of GLEW DLL")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${BINARY_DIR}/Release/zlib.lib CACHE FILEPATH "Location of ZLib release library")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG "" CACHE FILEPATH "Location of ZLib debug library")

endif ()