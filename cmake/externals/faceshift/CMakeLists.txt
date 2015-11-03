set(EXTERNAL_NAME faceshift)

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL https://hifi-public.s3.amazonaws.com/dependencies/faceshift.zip
  CMAKE_ARGS ${ANDROID_CMAKE_ARGS} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  BINARY_DIR ${EXTERNAL_PROJECT_PREFIX}/build
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)

#  URL_MD5 1bdcb8a0b8d5b1ede434cc41efade41d

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR}/include CACHE FILEPATH "Path to Faceshift include directory")

if (WIN32)
    set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${INSTALL_DIR}/lib/Debug/faceshift.lib CACHE FILEPATH "Faceshift libraries")
    set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/Release/faceshift.lib CACHE FILEPATH "Faceshift libraries")
elseif (APPLE)
    set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${INSTALL_DIR}/lib/Debug/libfaceshift.a CACHE FILEPATH "Faceshift libraries")
    set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/Release/libfaceshift.a CACHE FILEPATH "Faceshift libraries")
endif()
