if (WIN32)
  set(EXTERNAL_NAME glew)

  include(ExternalProject)
  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL http://hifi-public.s3.amazonaws.com/dependencies/glew-1.10.0-win32.zip
    URL_MD5 37514e4e595a3b3dc587eee8f7e8ec2f
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
  )

  # Hide this external target (for ide users)
  set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

  string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE PATH "List of glew include directories")

  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    set(_LIB_DIR ${SOURCE_DIR}/lib/Release/x64)
    set(${EXTERNAL_NAME_UPPER}_DLL_PATH ${SOURCE_DIR}/bin/Release/x64 CACHE FILEPATH "Location of GLEW DLL")
  else()
    set(_LIB_DIR ${SOURCE_DIR}/lib/Release/Win32)
    set(${EXTERNAL_NAME_UPPER}_DLL_PATH ${SOURCE_DIR}/bin/Release/Win32 CACHE FILEPATH "Location of GLEW DLL")
  endif()

  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${_LIB_DIR}/glew32.lib CACHE FILEPATH "Location of GLEW release library")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG "" CACHE FILEPATH "Location of GLEW debug library")

endif ()