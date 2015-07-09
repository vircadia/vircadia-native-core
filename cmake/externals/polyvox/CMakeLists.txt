set(EXTERNAL_NAME polyvox)

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL http://hifi-public.s3.amazonaws.com/dependencies/polyvox.zip
  URL_MD5 904b840328278c9b36fa7a14be730c34
  CMAKE_ARGS -DENABLE_EXAMPLES=OFF -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
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
  message(STATUS "in polyvox INSTALL_NAME_LIBRARY_DIR ${INSTALL_NAME_LIBRARY_DIR}")
  ExternalProject_Add_Step(
    ${EXTERNAL_NAME}
    change-install-name
    COMMENT "Calling install_name_tool on libraries to fix install name for dylib linking"
    COMMAND ${CMAKE_COMMAND} -DINSTALL_NAME_LIBRARY_DIR=${INSTALL_NAME_LIBRARY_DIR} -P ${EXTERNAL_PROJECT_DIR}/OSXInstallNameChange.cmake
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
  set(${EXTERNAL_NAME_UPPER}_CORE_LIBRARY ${INSTALL_DIR}/PolyVoxCore/lib/PolyVoxCore.lib CACHE FILEPATH "polyvox core library")
#  set(${EXTERNAL_NAME_UPPER}_UTIL_LIBRARY ${INSTALL_DIR}/PolyVoxUtil/lib/PolyVoxUtil.lib CACHE FILEPATH "polyvox util library")
elseif (APPLE)
set(${EXTERNAL_NAME_UPPER}_CORE_LIBRARY ${INSTALL_DIR}/lib/libPolyVoxCore.dylib CACHE FILEPATH "polyvox core library")
#  set(${EXTERNAL_NAME_UPPER}_UTIL_LIBRARY ${INSTALL_DIR}/lib/libPolyVoxUtil.dylib CACHE FILEPATH "polyvox util library")
else ()
  set(${EXTERNAL_NAME_UPPER}_CORE_LIBRARY ${INSTALL_DIR}/lib/libPolyVoxCore.so CACHE FILEPATH "polyvox core library")
#  set(${EXTERNAL_NAME_UPPER}_UTIL_LIBRARY ${INSTALL_DIR}/lib/libPolyVoxUtil.so CACHE FILEPATH "polyvox util library")
endif ()
