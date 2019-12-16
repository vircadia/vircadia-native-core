set(EXTERNAL_NAME tbb)

include(ExternalProject)

if (WIN32)
  set(DOWNLOAD_URL "${EXTERNAL_BUILD_ASSETS}/dependencies/tbb2017_20170604oss_win_slim.zip")
  set(DOWNLOAD_MD5 065934458e3db88397f3d10e7eea536c)
elseif (APPLE)
  set(DOWNLOAD_URL "${EXTERNAL_BUILD_ASSETS}/dependencies/tbb2017_20170604oss_mac_slim.tar.gz")
  set(DOWNLOAD_MD5 62bde626b396f8e1a85c6a8ded1d8105)
else ()
  set(DOWNLOAD_URL "${EXTERNAL_BUILD_ASSETS}/dependencies/tbb2017_20170604oss_lin_slim.tar.gz")
  set(DOWNLOAD_MD5 2a5c721f40fa3503ffc12c18dd00011c)
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
    DEPENDEES download
    WORKING_DIRECTORY <SOURCE_DIR>
    LOG 1
  )
elseif (WIN32)
  if (MSVC_VERSION GREATER_EQUAL 1900)
    set(_TBB_MSVC_DIR "vc14")
  elseif (MSVC_VERSION GREATER_EQUAL 1800)
    set(_TBB_MSVC_DIR "vc12")
  elseif (MSVC_VERSION GREATER_EQUAL 1700)
    set(_TBB_MSVC_DIR "vc11")
  else()
    message(FATAL_ERROR "MSVC ${MSVC_VERSION} not supported by Intel TBB")
  endif()

  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    set(_TBB_LIB_DIR "${SOURCE_DIR}/lib/intel64/${_TBB_MSVC_DIR}")
    set(${EXTERNAL_NAME_UPPER}_DLL_PATH "${SOURCE_DIR}/bin/intel64/${_TBB_MSVC_DIR}" CACHE PATH "Path to TBB DLLs")
  else()
    set(_TBB_LIB_DIR "${SOURCE_DIR}/lib/ia32/${_TBB_MSVC_DIR}")
    set(${EXTERNAL_NAME_UPPER}_DLL_PATH "${SOURCE_DIR}/bin/ia32/${_TBB_MSVC_DIR}" CACHE PATH "Path to TBB DLLs")
  endif()

  set(_LIB_EXT "lib")
elseif (ANDROID)
  set(_TBB_LIB_DIR "${SOURCE_DIR}/lib/android")
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

  if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)
    set(_TBB_LIB_DIR "${SOURCE_DIR}/lib/${_TBB_ARCH_DIR}/gcc4.7")
  elseif (GCC_VERSION VERSION_GREATER 4.4 OR GCC_VERSION VERSION_EQUAL 4.4)
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
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE STRING "List of tbb include directories")
endif ()

