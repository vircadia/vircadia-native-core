include(ExternalProject)
include(SelectLibraryConfigurations)

set(EXTERNAL_NAME Sixense)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL http://hifi-public.s3.amazonaws.com/dependencies/SixenseSDK_071615.zip
  URL_MD5 752a3901f334124e9cffc2ba4136ef7d
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD 1
)

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE TYPE INTERNAL)

if (WIN32)
  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
      set(ARCH_DIR "x64/VS2013")
      set(ARCH_SUFFIX "_x64")
  else()
      set(ARCH_DIR "Win32/VS2013")
      set(ARCH_SUFFIX "")
  endif()
  
  set(LIB_DIR "${SOURCE_DIR}/lib/${ARCH_DIR}")
  set(BIN_DIR "${SOURCE_DIR}/bin/${ARCH_DIR}")
  set(SIXENSE_LIBRARY_RELEASE "${LIB_DIR}/release_dll/sixense${ARCH_SUFFIX}.lib" CACHE FILEPATH "Sixense release lib")
  set(SIXENSE_LIBRARY_DEBUG "${LIB_DIR}/debug_dll/sixensed${ARCH_SUFFIX}.lib" CACHE FILEPATH "Sixense debug lib")
  set(SIXENSE_RELEASE_DLL_PATH "${BIN_DIR}/release_dll" CACHE FILEPATH "Sixense release DLL path")
  set(SIXENSE_DEBUG_DLL_PATH "${BIN_DIR}/debug_dll" CACHE FILEPATH "Sixense debug DLL path")

  # FIXME need to account for different architectures 
  add_paths_to_fixup_libs(${SIXENSE_DEBUG_DLL_PATH})
  add_paths_to_fixup_libs(${SIXENSE_RELEASE_DLL_PATH})

elseif(APPLE)

  set(ARCH_DIR "osx_x64")
  set(ARCH_SUFFIX "_x64")
  set(LIB_DIR "${SOURCE_DIR}/lib/${ARCH_DIR}")

  set(SIXENSE_LIBRARY_RELEASE "${LIB_DIR}/release_dll/libsixense${ARCH_SUFFIX}.dylib" CACHE FILEPATH "Sixense release lib")
  set(SIXENSE_LIBRARY_DEBUG "${LIB_DIR}/debug_dll/libsixensed${ARCH_SUFFIX}.dylib" CACHE FILEPATH "Sixense debug lib")

elseif(NOT ANDROID)

  set(ARCH_DIR "linux_x64")
  set(ARCH_SUFFIX "_x64")
  set(LIB_DIR "${SOURCE_DIR}/lib/${ARCH_DIR}")

  set(SIXENSE_LIBRARY_RELEASE "${LIB_DIR}/release/libsixense${ARCH_SUFFIX}.so" CACHE FILEPATH "Sixense release lib")
  set(SIXENSE_LIBRARY_DEBUG "${LIB_DIR}/debug/libsixensed${ARCH_SUFFIX}.so" CACHE FILEPATH "Sixense debug lib")

endif()

