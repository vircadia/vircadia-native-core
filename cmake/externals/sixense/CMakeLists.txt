include(ExternalProject)

set(EXTERNAL_NAME sixense)

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
    
    set(${EXTERNAL_NAME_UPPER}_LIBRARIES "${SOURCE_DIR}/lib/${ARCH_DIR}/VS2013/release_dll/sixense${ARCH_SUFFIX}.lib" CACHE TYPE INTERNAL)
    add_paths_to_fixup_libs("${SOURCE_DIR}/bin/${ARCH_DIR}/VS2013/release_dll")

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
    set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/linux_x64/release/libsixense_x64.so CACHE TYPE INTERNAL)

endif()

