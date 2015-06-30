include(ExternalProject)
include(SelectLibraryConfigurations)

set(EXTERNAL_NAME Sixense)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL ./SixenseSDK_062612.zip
  URL_MD5 10cc8dc470d2ac1244a88cf04bc549cc
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD 1
)

if (APPLE)
  find_library(SIXENSE_LIBRARY_RELEASE lib/osx_x64/release_dll/libsixense_x64.dylib HINTS ${SIXENSE_SEARCH_DIRS})
  find_library(SIXENSE_LIBRARY_DEBUG lib/osx_x64/debug_dll/libsixensed_x64.dylib HINTS ${SIXENSE_SEARCH_DIRS})
elseif (UNIX)
  find_library(SIXENSE_LIBRARY_RELEASE lib/linux_x64/release/libsixense_x64.so HINTS ${SIXENSE_SEARCH_DIRS})
  # find_library(SIXENSE_LIBRARY_DEBUG lib/linux_x64/debug/libsixensed_x64.so HINTS ${SIXENSE_SEARCH_DIRS})
elseif (WIN32)
endif ()



ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE TYPE INTERNAL)

if (WIN32)

    if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        set(ARCH_DIR "x64")
        set(ARCH_SUFFIX "_x64")
    else()
        set(ARCH_DIR "Win32")
        set(ARCH_SUFFIX "")
    endif()

  # FIXME need to account for different architectures 
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES "${SOURCE_DIR}/lib/${ARCH_DIR}/release_dll/sixense${ARCH_SUFFIX}.lib" CACHE TYPE INTERNAL)
  add_paths_to_fixup_libs(${SOURCE_DIR}/bin/win32)

elseif(APPLE)

  # FIXME need to account for different architectures 
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/osx32/libopenvr_api.dylib CACHE TYPE INTERNAL)
  add_paths_to_fixup_libs(${SOURCE_DIR}/bin/osx32)

elseif(NOT ANDROID)

  # FIXME need to account for different architectures 
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/linux32/libopenvr_api.so CACHE TYPE INTERNAL)
  add_paths_to_fixup_libs(${SOURCE_DIR}/bin/linux32)
  
endif()

