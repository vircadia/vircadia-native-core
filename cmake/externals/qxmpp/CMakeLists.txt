set(EXTERNAL_NAME qxmpp)

# we need to find qmake inside QT_DIR
find_program(QMAKE_COMMAND NAME qmake PATHS ${QT_DIR}/bin NO_DEFAULT_PATH)

if (NOT QMAKE_COMMAND)
  message(FATAL_ERROR "Could not find qmake. Qxmpp cannot be compiled without qmake.")
endif ()

if (ANDROID)
  set(ANDROID_CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}" "-DANDROID_NATIVE_API_LEVEL=19")
endif ()

if (WIN32)
  find_program(PLATFORM_BUILD_COMMAND nmake)
  if (NOT PLATFORM_BUILD_COMMAND)
    message(FATAL_ERROR "You asked us to grap QXmpp and build it, but nmake was not found. Please make sure the folder containing nmake.exe is in your PATH.")
  endif ()
else ()
  find_program(PLATFORM_BUILD_COMMAND make)
endif ()

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL http://qxmpp.googlecode.com/files/qxmpp-0.7.6.tar.gz
  URL_MD5 ee45a97313306ded2ff0f6618a3ed1e1
  BUILD_IN_SOURCE 1
  PATCH_COMMAND patch -p2 -t -N --verbose < ${CMAKE_CURRENT_SOURCE_DIR}/qxmpp.patch
  CONFIGURE_COMMAND ${QMAKE_COMMAND} PREFIX=<INSTALL_DIR>
  BUILD_COMMAND ${PLATFORM_BUILD_COMMAND}
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR}/include CACHE FILEPATH "Path to Qxmpp include directory")

if (WIN32)
  set(_LIB_EXT "lib")
  
  set(${EXTERNAL_NAME_UPPER}_DLL_PATH ${_LIB_DIR} CACHE PATH "Location of QXmpp DLL")
else ()
  if (APPLE)
    set(_LIB_EXT "dylib")
  else ()
    set(_LIB_EXT "so")
  endif ()
  
  set(_LIB_PREFIX "lib")
endif ()

set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/${_LIB_PREFIX}qxmpp.${_LIB_EXT} CACHE FILEPATH "Path to QXmpp release library")
set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG NOTFOUND CACHE FILEPATH "Path to QXmpp debug library")
