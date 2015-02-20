set(EXTERNAL_NAME sdl2)

include(ExternalProject)

if (WIN32)
  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL http://www.libsdl.org/release/SDL2-devel-2.0.3-VC.zip
    URL_MD5 30a333bcbe94bc5016e8799c73e86233
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
  )
else ()
  if (ANDROID)
    set(ANDROID_CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}" "-DANDROID_NATIVE_API_LEVEL=19")
  endif ()
  
  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL http://www.libsdl.org/release/SDL2-2.0.3.tar.gz
    URL_MD5 fe6c61d2e9df9ef570e7e80c6e822537
    CMAKE_ARGS ${ANDROID_CMAKE_ARGS} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    LOG_DOWNLOAD 1
    LOG_CONFIGURE 1
    LOG_BUILD 1
  )
endif ()

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (WIN32)
  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)
  set(_ROOT_DIR ${SOURCE_DIR})
  set(_LIB_DIR "${SOURCE_DIR}/lib/x86")
  set(_LIB_EXT "lib")
  
  set(${EXTERNAL_NAME_UPPER}_DLL_PATH ${_LIB_DIR} CACHE PATH "Location of SDL2 DLL")
else ()
  ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)
  set(_ROOT_DIR ${INSTALL_DIR})
  
  set(_LIB_DIR ${INSTALL_DIR}/lib)
  set(_LIB_EXT "so")
  set(_LIB_PREFIX "lib")
endif ()

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIR ${_ROOT_DIR}/include/SDL2 CACHE PATH "Location of SDL2 include directory")
set(${EXTERNAL_NAME_UPPER}_LIBRARY_TEMP ${_LIB_DIR}/${_LIB_PREFIX}SDL2.${_LIB_EXT} CACHE FILEPATH "Path to SDL2 library")