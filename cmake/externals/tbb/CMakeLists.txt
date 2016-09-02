set(EXTERNAL_NAME tbb)

include(ExternalProject)

if (ANDROID)

  find_program(NDK_BUILD_COMMAND NAMES ndk-build DOC "Path to the ndk-build command")

  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL http://hifi-public.s3.amazonaws.com/dependencies/tbb43_20150316oss_src.tgz
    URL_MD5 bf090eaa86cf89ea014b7b462786a440
    BUILD_COMMAND ${NDK_BUILD_COMMAND} --directory=jni target=android tbb tbbmalloc arch=arm
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -DTBB_LIBS_SUFFIX=so -P ${CMAKE_CURRENT_SOURCE_DIR}/TBBLibCopy.cmake
    LOG_DOWNLOAD 1
    LOG_CONFIGURE 1
    LOG_BUILD 1
  )
elseif (APPLE)
  find_program(MAKE_COMMAND NAMES make DOC "Path to the make command")

  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL http://hifi-public.s3.amazonaws.com/dependencies/tbb43_20150316oss_src.tgz
    URL_MD5 bf090eaa86cf89ea014b7b462786a440
    BUILD_COMMAND ${MAKE_COMMAND} tbb_os=macos
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -DTBB_LIBS_SUFFIX=dylib -P ${CMAKE_CURRENT_SOURCE_DIR}/TBBLibCopy.cmake
    LOG_DOWNLOAD 1
    LOG_CONFIGURE 1
    LOG_BUILD 1
  )
else ()
  if (WIN32)
    set(DOWNLOAD_URL http://s3.amazonaws.com/hifi-public/dependencies/tbb43_20150316oss_win.zip)
    set(DOWNLOAD_MD5 d250d40bb93b255f75bcbb19e976a440)
  else ()
    set(DOWNLOAD_URL http://s3.amazonaws.com/hifi-public/dependencies/tbb43_20150316oss_lin.tgz)
    set(DOWNLOAD_MD5 7830ba2bc62438325fba2ec0c95367a5)
  endif ()

  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL ${DOWNLOAD_URL}
    URL_MD5 ${DOWNLOAD_MD5}
    BUILD_COMMAND ""
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
  )
endif ()

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (APPLE)
  set(_TBB_LIB_DIR "${SOURCE_DIR}/lib")
  set(_LIB_PREFIX "lib")
  set(_LIB_EXT "dylib")

  ExternalProject_Add_Step(
    ${EXTERNAL_NAME}
    change-install-name
    COMMENT "Calling install_name_tool on TBB libraries to fix install name for dylib linking"
    COMMAND ${CMAKE_COMMAND} -DINSTALL_NAME_LIBRARY_DIR=${_TBB_LIB_DIR} -P ${EXTERNAL_PROJECT_DIR}/OSXInstallNameChange.cmake
    DEPENDEES install
    WORKING_DIRECTORY <SOURCE_DIR>
    LOG 1
  )

elseif (WIN32)
  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    set(_TBB_LIB_DIR "${SOURCE_DIR}/lib/intel64/vc12")
    set(${EXTERNAL_NAME_UPPER}_DLL_PATH "${SOURCE_DIR}/bin/intel64/vc12" CACHE PATH "Path to TBB DLLs")
  else()
    set(_TBB_LIB_DIR "${SOURCE_DIR}/lib/ia32/vc12")
    set(${EXTERNAL_NAME_UPPER}_DLL_PATH "${SOURCE_DIR}/bin/ia32/vc12" CACHE PATH "Path to TBB DLLs")
  endif()
  set(_LIB_EXT "lib")
elseif (ANDROID)
  set(_TBB_LIB_DIR "${SOURCE_DIR}/lib")
  set(_LIB_PREFIX "lib")
  set(_LIB_EXT "so")
elseif (UNIX)
  set(_LIB_PREFIX "lib")
  set(_LIB_EXT "so")

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_TBB_ARCH_DIR "intel64")
  else()
    set(_TBB_ARCH_DIR "ia32")
  endif()

  execute_process(
    COMMAND ${CMAKE_C_COMPILER} -dumpversion
    OUTPUT_VARIABLE GCC_VERSION
  )

  if (GCC_VERSION VERSION_GREATER 4.4 OR GCC_VERSION VERSION_EQUAL 4.4)
    set(_TBB_LIB_DIR "${SOURCE_DIR}/lib/${_TBB_ARCH_DIR}/gcc4.4")
  elseif (GCC_VERSION VERSION_GREATER 4.1 OR GCC_VERSION VERSION_EQUAL 4.1)
    set(_TBB_LIB_DIR "${SOURCE_DIR}/lib/${_TBB_ARCH_DIR}/gcc4.1")
  else ()
    message(STATUS "Could not find a compatible version of Threading Building Blocks library for your compiler.")
  endif ()


endif ()

if (DEFINED _TBB_LIB_DIR)
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${_TBB_LIB_DIR}/${_LIB_PREFIX}tbb_debug.${_LIB_EXT} CACHE FILEPATH "TBB debug library location")
  set(${EXTERNAL_NAME_UPPER}_MALLOC_LIBRARY_DEBUG ${_TBB_LIB_DIR}/${_LIB_PREFIX}tbbmalloc_debug.${_LIB_EXT} CACHE FILEPATH "TBB malloc debug library location")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${_TBB_LIB_DIR}/${_LIB_PREFIX}tbb.${_LIB_EXT} CACHE FILEPATH "TBB release library location")
  set(${EXTERNAL_NAME_UPPER}_MALLOC_LIBRARY_RELEASE ${_TBB_LIB_DIR}/${_LIB_PREFIX}tbbmalloc.${_LIB_EXT} CACHE FILEPATH "TBB malloc release library location")
endif ()

if (DEFINED ${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE)
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE TYPE "List of tbb include directories")
endif ()
