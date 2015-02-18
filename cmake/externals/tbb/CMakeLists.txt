set(EXTERNAL_NAME tbb)

if (APPLE)
  set(DOWNLOAD_URL https://www.threadingbuildingblocks.org/sites/default/files/software_releases/mac/tbb43_20150209oss_osx.tgz)
elseif (WIN32)
  set(DOWNLOAD_URL https://www.threadingbuildingblocks.org/sites/default/files/software_releases/windows/tbb43_20150209oss_win.zip)
else() 
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
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${SOURCE_DIR}/lib/libc++/libtbb_debug.dylib CACHE TYPE "TBB debug library location")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${SOURCE_DIR}/lib/libc++/libtbb.dylib CACHE TYPE "TBB release library location")
  set(${EXTERNAL_NAME_UPPER}_MALLOC_LIBRARY_DEBUG ${SOURCE_DIR}/lib/libc++/libtbbmalloc_debug.dylib CACHE TYPE "TBB malloc debug library location")
  set(${EXTERNAL_NAME_UPPER}_MALLOC_LIBRARY_RELEASE ${SOURCE_DIR}/lib/libc++/libtbbmalloc.dylib CACHE TYPE "TBB malloc release library location")
elseif (WIN32)
  # set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${INSTALL_DIR}/lib/soxr.lib CACHE TYPE "List of soxr libraries")
  # set(${EXTERNAL_NAME_UPPER}_DLL_PATH ${INSTALL_DIR}/bin CACHE TYPE "Path to soxr dll")
  
elseif (ANDROID)
  set(_TBB_ANDROID_LIB_PATH "${SOURCE_DIR}/lib/android/x86_64")
  
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${_TBB_ANDROID_LIB_PATH}/libtbb_debug.so CACHE TYPE "TBB debug library location")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${_TBB_ANDROID_LIB_PATH}/libtbb.so CACHE TYPE "TBB release library location")
  set(${EXTERNAL_NAME_UPPER}_MALLOC_LIBRARY_DEBUG ${_TBB_ANDROID_LIB_PATH}/libtbbmalloc_debug.so CACHE TYPE "TBB malloc debug library location")
  set(${EXTERNAL_NAME_UPPER}_MALLOC_LIBRARY_RELEASE ${_TBB_ANDROID_LIB_PATH}/libtbbmalloc.so CACHE TYPE "TBB malloc release library location")
elseif (UNIX)
  
else ()
  message(STATUS "Your OS is not handled by our TBB external project CMakeLists.txt. Please install TBB yourself and pass -DGET_TBB=0 when running CMake.")
endif () 