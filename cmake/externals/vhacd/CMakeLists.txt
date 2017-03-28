set(EXTERNAL_NAME vhacd)

if (ANDROID)
  set(ANDROID_CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}" "-DANDROID_NATIVE_API_LEVEL=19")
endif ()

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL http://hifi-public.s3.amazonaws.com/dependencies/v-hacd-master.zip
  URL_MD5 3bfc94f8dd3dfbfe8f4dc088f4820b3e
  CMAKE_ARGS ${ANDROID_CMAKE_ARGS} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  BINARY_DIR ${EXTERNAL_PROJECT_PREFIX}/build
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (WIN32)
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${INSTALL_DIR}/lib/Debug/VHACD_LIB.lib CACHE FILEPATH "Path to V-HACD debug library")

  # use generator expression to ensure the correct library is found when building different configurations in VS
  set(_LIB_FOLDER "$<$<CONFIG:RelWithDebInfo>:build/src/VHACD_Lib/RelWithDebInfo>")
  set(_LIB_FOLDER "${_LIB_FOLDER}$<$<CONFIG:MinSizeRel>:build/src/VHACD_Lib/MinSizeRel>")
  set(_LIB_FOLDER "${_LIB_FOLDER}$<$<OR:$<CONFIG:Release>,$<CONFIG:Debug>>:lib/Release>")

  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/${_LIB_FOLDER}/VHACD_LIB.lib CACHE FILEPATH "Path to V-HACD release library")
else ()
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG "" CACHE FILEPATH "Path to V-HACD debug library")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/libVHACD.a CACHE FILEPATH "Path to V-HACD release library")
endif ()

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR}/include CACHE FILEPATH "Path to V-HACD include directory")
