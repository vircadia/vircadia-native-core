set(EXTERNAL_NAME tbb)

include(ExternalProject)

if (ANDROID)
  
  find_program(NDK_BUILD_COMMAND NAMES ndk-build DOC "Path to the ndk-build command")
      
  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL http://hifi-public.s3.amazonaws.com/dependencies/tbb43_20150209oss_src.tgz
    BUILD_COMMAND ${NDK_BUILD_COMMAND} --directory=jni target=android tbb tbbmalloc arch=arm
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/AndroidTBBLibCopy.cmake
    LOG_DOWNLOAD ON
  )
else ()
  if (APPLE)
    set(DOWNLOAD_URL http://s3.amazonaws.com/hifi-public/dependencies/tbb43_20150209oss_osx.tgz)
  elseif (WIN32)
    set(DOWNLOAD_URL http://s3.amazonaws.com/hifi-public/dependencies/tbb43_20150209oss_win.zip)
  else () 
    set(DOWNLOAD_URL http://s3.amazonaws.com/hifi-public/dependencies/tbb43_20150209oss_lin.tgz)
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
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE TYPE "List of tbb include directories")

if (APPLE) 
  set(_TBB_APPLE_LIB_DIR "${SOURCE_DIR}/lib/libc++")
  
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${_TBB_APPLE_LIB_DIR}/libtbb_debug.dylib CACHE TYPE "TBB debug library location")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${_TBB_APPLE_LIB_DIR}/libtbb.dylib CACHE TYPE "TBB release library location")
  set(${EXTERNAL_NAME_UPPER}_MALLOC_LIBRARY_DEBUG ${_TBB_APPLE_LIB_DIR}/libtbbmalloc_debug.dylib CACHE TYPE "TBB malloc debug library location")
  set(${EXTERNAL_NAME_UPPER}_MALLOC_LIBRARY_RELEASE ${_TBB_APPLE_LIB_DIR}/libtbbmalloc.dylib CACHE TYPE "TBB malloc release library location")
elseif (WIN32)
  set(_TBB_WIN_LIB_DIR "${SOURCE_DIR}/lib/ia32/vc12")
  
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${_TBB_WIN_LIB_DIR}/tbb_debug.lib CACHE TYPE "TBB debug library location")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${_TBB_WIN_LIB_DIR}/tbb.lib CACHE TYPE "TBB release library location")
  set(${EXTERNAL_NAME_UPPER}_MALLOC_LIBRARY_DEBUG ${_TBB_WIN_LIB_DIR}/tbbmalloc_debug.lib CACHE TYPE "TBB malloc debug library location")
  set(${EXTERNAL_NAME_UPPER}_MALLOC_LIBRARY_RELEASE ${_TBB_WIN_LIB_DIR}/tbbmalloc.lib CACHE TYPE "TBB malloc release library location")
  
  set(${EXTERNAL_NAME_UPPER}_DLL_PATH "${SOURCE_DIR}/bin/ia32/vc12" CACHE TYPE "Path to TBB DLLs")
elseif (ANDROID)
  set(_TBB_ANDROID_LIB_DIR "${SOURCE_DIR}/lib")
  
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${_TBB_ANDROID_LIB_DIR}/libtbb_debug.so CACHE TYPE "TBB debug library location")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${_TBB_ANDROID_LIB_DIR}/libtbb.so CACHE TYPE "TBB release library location")
  set(${EXTERNAL_NAME_UPPER}_MALLOC_LIBRARY_DEBUG ${_TBB_ANDROID_LIB_DIR}/libtbbmalloc_debug.so CACHE TYPE "TBB malloc debug library location")
  set(${EXTERNAL_NAME_UPPER}_MALLOC_LIBRARY_RELEASE ${_TBB_ANDROID_LIB_DIR}/libtbbmalloc.so CACHE TYPE "TBB malloc release library location")
elseif (UNIX)
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
    set(_TBB_LINUX_LIB_DIR "${SOURCE_DIR}/lib/${_TBB_ARCH_DIR}/gcc4.4")
  elseif (GCC_VERSION VERSION_GREATER 4.1 OR GCC_VERSION VERSION_EQUAL 4.1)
    set(_TBB_LINUX_LIB_DIR "${SOURCE_DIR}/lib/${_TBB_ARCH_DIR}/gcc4.1")
  else ()
    message(FATAL_ERROR "Could not find a compatible version of Threading Building Blocks library for your compiler.")
  endif ()
  
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${_TBB_LINUX_LIB_DIR}/libtbb_debug.so CACHE TYPE "TBB debug library location")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${_TBB_LINUX_LIB_DIR}/libtbb.so CACHE TYPE "TBB release library location")
  set(${EXTERNAL_NAME_UPPER}_MALLOC_LIBRARY_DEBUG ${_TBB_LINUX_LIB_DIR}/libtbbmalloc_debug.so CACHE TYPE "TBB malloc debug library location")
  set(${EXTERNAL_NAME_UPPER}_MALLOC_LIBRARY_RELEASE ${_TBB_LINUX_LIB_DIR}/libtbbmalloc.so CACHE TYPE "TBB malloc release library location")
  
else ()
  message(STATUS "Your OS is not handled by our TBB external project CMakeLists.txt. Please install TBB yourself and pass -DGET_TBB=0 when running CMake.")
endif () 