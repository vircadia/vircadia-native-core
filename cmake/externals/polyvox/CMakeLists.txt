set(EXTERNAL_NAME polyvox)

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  # URL http://hifi-public.s3.amazonaws.com/dependencies/polyvox-master.zip
  URL http://headache.hungry.com/~seth/hifi/polyvox-master.zip
  URL_MD5 ceabe53ccb0726524e1d0427c517f3b5
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  BINARY_DIR ${EXTERNAL_PROJECT_PREFIX}/build
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

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
  set(${EXTERNAL_NAME_UPPER}_CORE_LIBRARY ${INSTALL_DIR}/lib/polyvox.lib CACHE FILEPATH "polyvox core library")
#  set(${EXTERNAL_NAME_UPPER}_UTIL_LIBRARY ${INSTALL_DIR}/lib/polyvox.lib CACHE FILEPATH "polyvox util library")
else ()
  set(${EXTERNAL_NAME_UPPER}_CORE_LIBRARY ${INSTALL_DIR}/lib/libPolyVoxCore.so CACHE FILEPATH "polyvox core library")
#  set(${EXTERNAL_NAME_UPPER}_UTIL_LIBRARY ${INSTALL_DIR}/lib/libPolyVoxUtil.so CACHE FILEPATH "polyvox util library")
endif ()
