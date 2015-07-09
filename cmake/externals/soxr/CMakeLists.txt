set(EXTERNAL_NAME soxr)

if (ANDROID)
  set(PLATFORM_CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}" "-DANDROID_NATIVE_API_LEVEL=19" "-DHAVE_WORDS_BIGENDIAN_EXITCODE=1")
endif ()

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL http://hifi-public.s3.amazonaws.com/dependencies/soxr-0.1.1.zip
  URL_MD5 349b5b2f323a7380bc12186d98c77d1d
  CMAKE_ARGS ${PLATFORM_CMAKE_ARGS} -DBUILD_SHARED_LIBS=1 -DBUILD_TESTS=0 -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
  BINARY_DIR ${EXTERNAL_PROJECT_PREFIX}/build
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR}/include CACHE PATH "List of soxr include directories")

if (WIN32) 
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${INSTALL_DIR}/lib/soxr.lib CACHE FILEPATH "List of soxr libraries")
  set(${EXTERNAL_NAME_UPPER}_DLL_PATH ${INSTALL_DIR}/bin CACHE PATH "Path to soxr dll")
elseif (APPLE)
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${INSTALL_DIR}/lib/libsoxr.dylib CACHE FILEPATH "List of soxr libraries")
else ()
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${INSTALL_DIR}/lib/libsoxr.so CACHE FILEPATH "List of soxr libraries")
endif ()