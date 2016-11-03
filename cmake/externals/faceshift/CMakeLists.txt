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

set(LIBRARY_DEBUG_PATH "lib/Debug")
set(LIBRARY_RELEASE_PATH "lib/Release")

if (WIN32)
  set(LIBRARY_PREFIX "")
  set(LIBRARY_EXT "lib")
elseif (APPLE)
  set(LIBRARY_EXT "a")
  set(LIBRARY_PREFIX "lib")

  if (CMAKE_GENERATOR STREQUAL "Unix Makefiles")
    set(LIBRARY_DEBUG_PATH "build")
    set(LIBRARY_RELEASE_PATH "build")
  endif ()
endif()

set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG
  ${INSTALL_DIR}/${LIBRARY_DEBUG_PATH}/${LIBRARY_PREFIX}faceshift.${LIBRARY_EXT} CACHE FILEPATH "Faceshift libraries")
set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE
${INSTALL_DIR}/${LIBRARY_RELEASE_PATH}/${LIBRARY_PREFIX}faceshift.${LIBRARY_EXT} CACHE FILEPATH "Faceshift libraries")
