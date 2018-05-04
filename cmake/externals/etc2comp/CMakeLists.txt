set(EXTERNAL_NAME etc2comp)

if (ANDROID)
  set(ANDROID_CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}" "-DANDROID_NATIVE_API_LEVEL=19")
endif ()

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL https://hifi-public.s3.amazonaws.com/dependencies/etc2comp-master.zip
  URL_MD5 d0969e14af6b10b3306f880c8e2b1184
  CMAKE_ARGS ${ANDROID_CMAKE_ARGS}
  BINARY_DIR ${EXTERNAL_PROJECT_PREFIX}/build
  INSTALL_COMMAND ""
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)
ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (WIN32)
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${INSTALL_DIR}/build/EtcLib/Debug/EtcLib.lib CACHE FILEPATH "Path to Etc2Comp debug library")

  # use generator expression to ensure the correct library is found when building different configurations in VS
  set(_LIB_FOLDER "$<$<CONFIG:RelWithDebInfo>:build/EtcLib/RelWithDebInfo>")
  set(_LIB_FOLDER "${_LIB_FOLDER}$<$<CONFIG:MinSizeRel>:build/EtcLib/MinSizeRel>")
  set(_LIB_FOLDER "${_LIB_FOLDER}$<$<OR:$<CONFIG:Release>,$<CONFIG:Debug>>:build/EtcLib/Release>")

  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/${_LIB_FOLDER}/EtcLib.lib CACHE FILEPATH "Path to Etc2Comp release library")
else ()
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG "" CACHE FILEPATH "Path to EtcLib debug library")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/build/EtcLib/libEtcLib.a CACHE FILEPATH "Path to EtcLib release library")
endif ()

set(ETC_INCLUDE_DIR ${SOURCE_DIR}/EtcLib/Etc CACHE FILEPATH "Path to Etc2Comp/Etc include directory")
set(ETCCODEC_INCLUDE_DIR ${SOURCE_DIR}/EtcLib/EtcCodec CACHE FILEPATH "Path to Etc2Comp/EtcCodec include directory")
# ETC2COMP_INCLUDE_DIRS will be set later by FindEtc2Comp