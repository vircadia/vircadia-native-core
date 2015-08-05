set(EXTERNAL_NAME glew)

if (ANDROID)
  set(ANDROID_CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}" "-DANDROID_NATIVE_API_LEVEL=19")
endif ()

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL http://hifi-public.s3.amazonaws.com/dependencies/glew_simple.zip
  URL_MD5 95a97d3690aa6ba8f1cf5941880efbf6
  CONFIGURE_COMMAND CMAKE_ARGS ${ANDROID_CMAKE_ARGS} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR}/include CACHE PATH "List of glew include directories")

if (WIN32)
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${INSTALL_DIR}/lib/glew_d.lib CACHE FILEPATH "Path to glew debug library")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/glew.lib CACHE FILEPATH "Path to glew release library")
else ()
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/libglew.a CACHE FILEPATH "Path to glew release library")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG "" CACHE FILEPATH "Path to glew debug library")
endif ()