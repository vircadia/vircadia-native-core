include(ExternalProject)

set(EXTERNAL_NAME sixense)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

#set(SIXENSE_URL "https://hifi-public.s3.amazonaws.com/dependencies/SixenseSDK_062612.zip")
#set(SIXENSE_URL_MD5 "10cc8dc470d2ac1244a88cf04bc549cc")
#set(SIXENSE_NEW_LAYOUT 0)

#set(SIXENSE_URL "https://public.s3.amazonaws.com/dependencies/SixenseSDK_071615.zip")
#set(SIXENSE_URL_MD5 "752a3901f334124e9cffc2ba4136ef7d")
#set(SIXENSE_NEW_LAYOUT 1)

set(SIXENSE_URL "https://hifi-public.s3.amazonaws.com/dependencies/SixenseSDK_102215.zip")
set(SIXENSE_URL_MD5 "93c3a6795cce777a0f472b09532935f1")
set(SIXENSE_NEW_LAYOUT 1)

ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL ${SIXENSE_URL}
  URL_MD5 ${SIXENSE_URL_MD5}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD 1
)

set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

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
    
    if (${SIXENSE_NEW_LAYOUT})
        # for 2015 SDKs (using the 2013 versions may be causing the crash)
        set(${EXTERNAL_NAME_UPPER}_DLL_PATH "${SOURCE_DIR}/bin/${ARCH_DIR}/VS2010/release_dll")
        set(${EXTERNAL_NAME_UPPER}_LIB_PATH "${SOURCE_DIR}/lib/${ARCH_DIR}/VS2010/release_dll")
    else()
        # for the 2012 SDK
        set(${EXTERNAL_NAME_UPPER}_DLL_PATH "${SOURCE_DIR}/bin/${ARCH_DIR}/release_dll")
        set(${EXTERNAL_NAME_UPPER}_LIB_PATH "${SOURCE_DIR}/lib/${ARCH_DIR}/release_dll")
    endif()

    set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE "${${EXTERNAL_NAME_UPPER}_LIB_PATH}/sixense${ARCH_SUFFIX}.lib" CACHE TYPE INTERNAL)
    add_paths_to_fixup_libs("${${EXTERNAL_NAME_UPPER}_DLL_PATH}")

elseif(APPLE)

    set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${SOURCE_DIR}/lib/osx_x64/release_dll/libsixense_x64.dylib CACHE TYPE INTERNAL)
    set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${SOURCE_DIR}/lib/osx_x64/debug_dll/libsixensed_x64.dylib CACHE TYPE INTERNAL)
  
    set(_SIXENSE_LIB_DIR "${SOURCE_DIR}/lib/osx_x64")
    ExternalProject_Add_Step(
      ${EXTERNAL_NAME}
      change-install-name-release
      COMMENT "Calling install_name_tool on libraries to fix install name for dylib linking"
      COMMAND ${CMAKE_COMMAND} -DINSTALL_NAME_LIBRARY_DIR=${_SIXENSE_LIB_DIR}/release_dll -P ${EXTERNAL_PROJECT_DIR}/OSXInstallNameChange.cmake
      DEPENDEES install
      WORKING_DIRECTORY <SOURCE_DIR>
      LOG 1
    )

    set(_SIXENSE_LIB_DIR "${SOURCE_DIR}/lib/osx_x64")
    ExternalProject_Add_Step(
      ${EXTERNAL_NAME}
      change-install-name-debug
      COMMENT "Calling install_name_tool on libraries to fix install name for dylib linking"
      COMMAND ${CMAKE_COMMAND} -DINSTALL_NAME_LIBRARY_DIR=${_SIXENSE_LIB_DIR}/debug_dll -P ${EXTERNAL_PROJECT_DIR}/OSXInstallNameChange.cmake
      DEPENDEES install
      WORKING_DIRECTORY <SOURCE_DIR>
      LOG 1
    )
    
elseif(NOT ANDROID)

    # FIXME need to account for different architectures 
    set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${SOURCE_DIR}/lib/linux_x64/release/libsixense_x64.so CACHE TYPE INTERNAL)

endif()

