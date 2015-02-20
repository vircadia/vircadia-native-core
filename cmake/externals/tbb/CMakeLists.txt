set(EXTERNAL_NAME tbb)

include(ExternalProject)

if (ANDROID)
  
  find_program(NDK_BUILD_COMMAND NAMES ndk-build DOC "Path to the ndk-build command")
      
  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL http://hifi-public.s3.amazonaws.com/dependencies/tbb43_20150209oss_src.tgz
    URL_MD5 f09c9abe8ec74e6558c1f89cebbe2893
    BUILD_COMMAND ${NDK_BUILD_COMMAND} --directory=jni target=android tbb tbbmalloc arch=arm
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/AndroidTBBLibCopy.cmake
    LOG_DOWNLOAD 1
    LOG_CONFIGURE 1
    LOG_BUILD 1
  )
else ()
  if (APPLE)
    set(DOWNLOAD_URL http://s3.amazonaws.com/hifi-public/dependencies/tbb43_20150209oss_osx.tgz)
    set(DOWNLOAD_MD5 3e683c19792582b61382e0d760ea5db2)
  elseif (WIN32)
    set(DOWNLOAD_URL http://s3.amazonaws.com/hifi-public/dependencies/tbb43_20150209oss_win.zip)
    set(DOWNLOAD_MD5 e19c184f2bb0e944fc5f397f1e34ca84)
  else () 
    set(DOWNLOAD_URL http://s3.amazonaws.com/hifi-public/dependencies/tbb43_20150209oss_lin.tgz)
    set(DOWNLOAD_MD5 d9c2a6f7807df364be44a8c3c05e8457)
  endif ()
  
  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL ${DOWNLOAD_URL}
    BUILD_COMMAND ""
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
  )
endif ()

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (APPLE) 
  set(_TBB_LIB_DIR "${SOURCE_DIR}/lib/libc++")
  set(_LIB_PREFIX "lib")
  set(_LIB_EXT "dylib")
  
  ExternalProject_Add_Step(
    ${EXTERNAL_NAME}
    change-install-name
    COMMENT "Calling install_name_tool on TBB libraries to fix install name for dylib linking"
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/OSXTBBInstallNameChange.cmake
    DEPENDEES install
    WORKING_DIRECTORY <SOURCE_DIR>
    LOG 1
  )
  
elseif (WIN32)
  set(_TBB_LIB_DIR "${SOURCE_DIR}/lib/ia32/vc12")
  set(_LIB_EXT "lib")
  set(${EXTERNAL_NAME_UPPER}_DLL_PATH "${SOURCE_DIR}/bin/ia32/vc12" CACHE PATH "Path to TBB DLLs")
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
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${_TBB_LIB_DIR}/${_LIB_PREFIX}tbb.${_LIB_EXT} CACHE FILEPATH "TBB release library location")
  set(${EXTERNAL_NAME_UPPER}_MALLOC_LIBRARY_DEBUG ${_TBB_LIB_DIR}/${_LIB_PREFIX}tbbmalloc_debug.${_LIB_EXT} CACHE FILEPATH "TBB malloc debug library location")
  set(${EXTERNAL_NAME_UPPER}_MALLOC_LIBRARY_RELEASE ${_TBB_LIB_DIR}/${_LIB_PREFIX}tbbmalloc.${_LIB_EXT} CACHE FILEPATH "TBB malloc release library location")
endif ()

if (DEFINED ${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE)
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE TYPE "List of tbb include directories")
endif ()