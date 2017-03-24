set(EXTERNAL_NAME polyvox)

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL http://hifi-public.s3.amazonaws.com/dependencies/polyvox-master-2015-7-15.zip
  URL_MD5 9ec6323b87e849ae36e562ae1c7494a9
  CMAKE_ARGS -DENABLE_EXAMPLES=OFF -DENABLE_BINDINGS=OFF -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  BINARY_DIR ${EXTERNAL_PROJECT_PREFIX}/build
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

if (APPLE)
  set(INSTALL_NAME_LIBRARY_DIR ${INSTALL_DIR}/lib)

  ExternalProject_Add_Step(
    ${EXTERNAL_NAME}
    change-install-name-debug
    COMMENT "Calling install_name_tool on libraries to fix install name for dylib linking"
    COMMAND ${CMAKE_COMMAND} -DINSTALL_NAME_LIBRARY_DIR=${INSTALL_NAME_LIBRARY_DIR}/Debug -P ${EXTERNAL_PROJECT_DIR}/OSXInstallNameChange.cmake
    DEPENDEES install
    WORKING_DIRECTORY <SOURCE_DIR>
    LOG 1
  )

  ExternalProject_Add_Step(
    ${EXTERNAL_NAME}
    change-install-name-release
    COMMENT "Calling install_name_tool on libraries to fix install name for dylib linking"
    COMMAND ${CMAKE_COMMAND} -DINSTALL_NAME_LIBRARY_DIR=${INSTALL_NAME_LIBRARY_DIR}/Release -P ${EXTERNAL_PROJECT_DIR}/OSXInstallNameChange.cmake
    DEPENDEES install
    WORKING_DIRECTORY <SOURCE_DIR>
    LOG 1
  )
endif ()


string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (WIN32)
  set(${EXTERNAL_NAME_UPPER}_CORE_INCLUDE_DIRS ${INSTALL_DIR}/PolyVoxCore/include CACHE FILEPATH
    "Path to polyvox core include directory")
  set(${EXTERNAL_NAME_UPPER}_UTIL_INCLUDE_DIRS ${INSTALL_DIR}/PolyVoxUtil/include CACHE FILEPATH
    "Path to polyvox util include directory")
else ()
  set(${EXTERNAL_NAME_UPPER}_CORE_INCLUDE_DIRS ${INSTALL_DIR}/include/PolyVoxCore CACHE FILEPATH
    "Path to polyvox core include directory")
  set(${EXTERNAL_NAME_UPPER}_UTIL_INCLUDE_DIRS ${INSTALL_DIR}/include/PolyVoxUtil CACHE FILEPATH
    "Path to polyvox util include directory")
endif ()


if (WIN32)
  set(${EXTERNAL_NAME_UPPER}_CORE_LIBRARY_DEBUG ${INSTALL_DIR}/PolyVoxCore/lib/Debug/PolyVoxCore.lib CACHE FILEPATH "polyvox core library")

  # use generator expression to ensure the correct library is found when building different configurations in VS
  set(_LIB_FOLDER "$<$<CONFIG:RelWithDebInfo>:PolyVoxCore/lib/RelWithDebInfo>")
  set(_LIB_FOLDER "${_LIB_FOLDER}$<$<CONFIG:MinSizeRel>:build/library/PolyVoxCore/MinSizeRel>")
  set(_LIB_FOLDER "${_LIB_FOLDER}$<$<OR:$<CONFIG:Release>,$<CONFIG:Debug>>:PolyVoxCore/lib/Release>")

  set(${EXTERNAL_NAME_UPPER}_CORE_LIBRARY_RELEASE "${INSTALL_DIR}/${_LIB_FOLDER}/PolyVoxCore.lib" CACHE FILEPATH "polyvox core library")
#  set(${EXTERNAL_NAME_UPPER}_UTIL_LIBRARY ${INSTALL_DIR}/PolyVoxUtil/lib/PolyVoxUtil.lib CACHE FILEPATH "polyvox util library")
elseif (APPLE)
  set(${EXTERNAL_NAME_UPPER}_CORE_LIBRARY_DEBUG ${INSTALL_DIR}/lib/Debug/libPolyVoxCore.dylib CACHE FILEPATH "polyvox core library")
  set(${EXTERNAL_NAME_UPPER}_CORE_LIBRARY_RELEASE ${INSTALL_DIR}/lib/Release/libPolyVoxCore.dylib CACHE FILEPATH "polyvox core library")
#  set(${EXTERNAL_NAME_UPPER}_UTIL_LIBRARY ${INSTALL_DIR}/lib/libPolyVoxUtil.dylib CACHE FILEPATH "polyvox util library")
else ()
  set(${EXTERNAL_NAME_UPPER}_CORE_LIBRARY_DEBUG ${INSTALL_DIR}/lib/Debug/libPolyVoxCore.so CACHE FILEPATH "polyvox core library")
  set(${EXTERNAL_NAME_UPPER}_CORE_LIBRARY_RELEASE ${INSTALL_DIR}/lib/Release/libPolyVoxCore.so CACHE FILEPATH "polyvox core library")
#  set(${EXTERNAL_NAME_UPPER}_UTIL_LIBRARY ${INSTALL_DIR}/lib/libPolyVoxUtil.so CACHE FILEPATH "polyvox util library")
endif ()
