set(EXTERNAL_NAME sdl2)

include(ExternalProject)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (WIN32)
  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL http://hifi-public.s3.amazonaws.com/dependencies/SDL2-devel-2.0.3-VC.zip
    URL_MD5 30a333bcbe94bc5016e8799c73e86233
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
  )
elseif (APPLE)

    ExternalProject_Add(
      ${EXTERNAL_NAME}
      URL http://hifi-public.s3.amazonaws.com/dependencies/SDL2-2.0.3.zip
      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DVIDEO_OPENGL=OFF
      BINARY_DIR ${EXTERNAL_PROJECT_PREFIX}/build
      LOG_DOWNLOAD 1
      LOG_CONFIGURE 1
      LOG_BUILD 1
    )

    ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)
    set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIR ${INSTALL_DIR}/include/SDL2 CACHE PATH "Location of SDL2 include directory")
    set(${EXTERNAL_NAME_UPPER}_LIBRARY "${INSTALL_DIR}/lib/libSDL2-2.0.dylib" CACHE STRING "Path to SDL2 library")

    set(_SDL2_LIB_DIR "${INSTALL_DIR}/lib")

    ExternalProject_Add_Step(
      ${EXTERNAL_NAME}
      change-install-name
      COMMENT "Calling install_name_tool on SDL2 libraries to fix install name for dylib linking"
      COMMAND ${CMAKE_COMMAND} -DINSTALL_NAME_LIBRARY_DIR=${_SDL2_LIB_DIR} -P ${EXTERNAL_PROJECT_DIR}/OSXInstallNameChange.cmake
      DEPENDEES install
      WORKING_DIRECTORY <INSTALL_DIR>
      LOG 1
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

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

if (APPLE)

# NOOP

elseif (WIN32)

  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIR ${SOURCE_DIR}/include CACHE PATH "Location of SDL2 include directory")

  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    set(${EXTERNAL_NAME_UPPER}_LIBRARY_TEMP ${SOURCE_DIR}/lib/x64/SDL2.lib CACHE FILEPATH "Path to SDL2 library")
    set(${EXTERNAL_NAME_UPPER}_DLL_PATH ${SOURCE_DIR}/lib/x64 CACHE PATH "Location of SDL2 DLL")
  else()
    set(${EXTERNAL_NAME_UPPER}_LIBRARY_TEMP ${SOURCE_DIR}/lib/x86/SDL2.lib CACHE FILEPATH "Path to SDL2 library")
    set(${EXTERNAL_NAME_UPPER}_DLL_PATH ${SOURCE_DIR}/lib/x86 CACHE PATH "Location of SDL2 DLL")
  endif()

  add_paths_to_fixup_libs(${${EXTERNAL_NAME_UPPER}_DLL_PATH})

else ()

  ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIR ${INSTALL_DIR}/include/SDL2 CACHE PATH "Location of SDL2 include directory")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_TEMP ${INSTALL_DIR}/lib/libSDL2.so CACHE FILEPATH "Path to SDL2 library")

endif ()
