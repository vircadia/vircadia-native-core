set(EXTERNAL_NAME gverb)

if (ANDROID)
  set(ANDROID_CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}" "-DANDROID_NATIVE_API_LEVEL=19")
endif ()

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL http://hifi-public.s3.amazonaws.com/dependencies/gverb-master.zip
  URL_MD5 8b16d586390a2102804e46b87820dfc6
  CMAKE_ARGS ${ANDROID_CMAKE_ARGS} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR}/include CACHE TYPE "Path to gverb include directory")

if (WIN32) 
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${INSTALL_DIR}/lib/gverb.lib CACHE TYPE "List of gverb libraries")
else ()
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${INSTALL_DIR}/lib/libgverb.a CACHE TYPE "List of gverb libraries")
endif ()