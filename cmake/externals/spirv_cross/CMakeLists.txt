set(EXTERNAL_NAME spirv_cross)

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL https://github.com/KhronosGroup/SPIRV-Cross/archive/2018-08-07.zip
  URL_MD5 11198e4dc6a815ffbdb7a0a56d2d9261
  CONFIGURE_COMMAND CMAKE_ARGS  ${ANDROID_CMAKE_ARGS} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>-$<CONFIG> ${EXTRA_CMAKE_FLAGS}
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)
set(SUFFIXED_INSTALL_DIR "${INSTALL_DIR}-$<CONFIG>")

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SUFFIXED_INSTALL_DIR}/include CACHE PATH "List of Draco include directories")

if (UNIX)
  set(LIB_PREFIX "lib")
  set(LIB_EXT "a")
elseif (WIN32)
  set(LIB_EXT "lib")
endif ()

foreach(lib glsl msl cpp hlsl reflect util core)
    list(APPEND ${EXTERNAL_NAME_UPPER}_LIBRARIES ${SUFFIXED_INSTALL_DIR}/lib/spirv-cross-${lib}.${LIB_EXT})
endforeach()

set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${${EXTERNAL_NAME_UPPER}_LIBRARIES} CACHE FILEPATH "Path to SPIRV-Cross libraries")
