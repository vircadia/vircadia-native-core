set(EXTERNAL_NAME tbb)

if (APPLE)
  set(DOWNLOAD_URL https://www.threadingbuildingblocks.org/sites/default/files/software_releases/mac/tbb43_20150209oss_osx.tgz)
elseif (WIN32)
  set(DOWNLOAD_URL https://www.threadingbuildingblocks.org/sites/default/files/software_releases/windows/tbb43_20150209oss_win.zip)
else () 
  set(DOWNLOAD_URL https://www.threadingbuildingblocks.org/sites/default/files/software_releases/linux/tbb43_20150209oss_lin.tgz)
endif ()

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL ${DOWNLOAD_URL}
  BUILD_COMMAND ""
  CONFIGURE_COMMAND ""
  INSTALL_COMMAND ""
  UPDATE_COMMAND ""
  LOG_DOWNLOAD ON
)

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
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${_TBB_WIN_LIB_DIR}/libtbb_debug.lib CACHE TYPE "TBB debug library location")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${_TBB_WIN_LIB_DIR}/libtbb.lib CACHE TYPE "TBB release library location")
  set(${EXTERNAL_NAME_UPPER}_MALLOC_LIBRARY_DEBUG ${_TBB_WIN_LIB_DIR}/libtbbmalloc_debug.lib CACHE TYPE "TBB malloc debug library location")
  set(${EXTERNAL_NAME_UPPER}_MALLOC_LIBRARY_RELEASE ${_TBB_WIN_LIB_DIR}/libtbbmalloc.lib CACHE TYPE "TBB malloc release library location")
  
  set(${EXTERNAL_NAME_UPPER}_DLL_PATH "${SOURCE_DIR}/bin/ia32/vc12" CACHE TYPE "Path to TBB DLLs")
elseif (ANDROID)
  set(_TBB_ANDROID_LIB_DIR "${SOURCE_DIR}/lib/android/x86_64")
  
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${_TBB_ANDROID_LIB_DIR}/libtbb_debug.so CACHE TYPE "TBB debug library location")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${_TBB_ANDROID_LIB_DIR}/libtbb.so CACHE TYPE "TBB release library location")
  set(${EXTERNAL_NAME_UPPER}_MALLOC_LIBRARY_DEBUG ${_TBB_ANDROID_LIB_DIR}/libtbbmalloc_debug.so CACHE TYPE "TBB malloc debug library location")
  set(${EXTERNAL_NAME_UPPER}_MALLOC_LIBRARY_RELEASE ${_TBB_ANDROID_LIB_DIR}/libtbbmalloc.so CACHE TYPE "TBB malloc release library location")
elseif (UNIX)
  
else ()
  message(STATUS "Your OS is not handled by our TBB external project CMakeLists.txt. Please install TBB yourself and pass -DGET_TBB=0 when running CMake.")
endif () 