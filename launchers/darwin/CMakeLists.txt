cmake_minimum_required(VERSION 3.0)
project(HQLauncher)
set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")
set(src_files
  src/Launcher.h
  src/Launcher.m
  src/SplashScreen.h
  src/SplashScreen.m
  src/LoginScreen.h
  src/LoginScreen.m
  src/DisplayNameScreen.h
  src/DisplayNameScreen.m
  src/ProcessScreen.h
  src/ProcessScreen.m
  src/Window.h
  src/Window.m
  src/DownloadInterface.h
  src/DownloadInterface.m
  src/DownloadDomainContent.h
  src/DownloadDomainContent.m
  src/DownloadScripts.h
  src/DownloadScripts.m
  src/CredentialsRequest.h
  src/CredentialsRequest.m
  src/LatestBuildRequest.h
  src/LatestBuildRequest.m
  src/OrganizationRequest.m
  src/OrganizationRequest.h
  src/ErrorViewController.h
  src/ErrorViewController.m
  src/Settings.h
  src/Settings.m
  src/LaunchInterface.h
  src/CustomUI.h
  src/CustomUI.m
  src/main.mm
  nib/Window.xib
  nib/SplashScreen.xib
  nib/ErrorScreen.xib
  nib/LoginScreen.xib
  nib/ProcessScreen.xib
  nib/DisplayNameScreen.xib)

set(APP_NAME "HQ Launcher")

set(CMAKE_C_FLAGS "-x objective-c")
set(CMAKE_CXX_FLAGS "-x objective-c++")
set(CMAKE_EXE_LINKER_FLAGS "-framework Cocoa -framework AppKit -framework QuartzCore")

set_target_properties(${this_target} PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST MacOSXBundleInfo.plist.in)


set(MACOSX_BUNDLE_ICON_FILE "interface.icns")

function(set_from_env _RESULT_NAME _ENV_VAR_NAME _DEFAULT_VALUE)
    if (NOT DEFINED ${_RESULT_NAME}) 
        if ("$ENV{${_ENV_VAR_NAME}}" STREQUAL "")
            set (${_RESULT_NAME} ${_DEFAULT_VALUE} PARENT_SCOPE)
        else()
            set (${_RESULT_NAME} $ENV{${_ENV_VAR_NAME}} PARENT_SCOPE)
        endif()
    endif()
endfunction()

add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${src_files})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${APP_NAME})
set_from_env(LAUNCHER_HMAC_SECRET LAUNCHER_HMAC_SECRET "")
target_compile_definitions(${PROJECT_NAME} PRIVATE LAUNCHER_HMAC_SECRET="${LAUNCHER_HMAC_SECRET}")

file(GLOB NIB_FILES "nib/*.xib")

find_program(IBTOOL ibtool HINTS "/usr/bin" "${OSX_DEVELOPER_ROOT}/usr/bin")
foreach (nibFile ${NIB_FILES})
  get_filename_component(fileWithExtension ${nibFile} NAME)
  string(REGEX REPLACE "\\.[^.]*$" "" file ${fileWithExtension})
  add_custom_command (TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${IBTOOL} --errors --warnings --notices
    --output-format human-readable-text
    --compile
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${APP_NAME}.app/Contents/Resources/${file}.nib"

    ${CMAKE_CURRENT_SOURCE_DIR}/nib/${file}.xib)
endforeach()


add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/images "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${APP_NAME}.app/Contents/Resources/")

install(
  TARGETS HQLauncher
  BUNDLE DESTINATION "."
  COMPONENT applications
)


set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})

include(CPackComponent)

set(CPACK_PACKAGE_NAME "HQ Launcher")
set(CPACK_PACKAGE_VENDOR "High Fidelity")
set(CPACK_PACKAGE_VERSION ${BUILD_VERSION})
set(CPACK_PACKAGE_FILE_NAME "HQ Launcher")

set(CPACK_NSIS_DISPLAY_NAME ${_DISPLAY_NAME})

set(DMG_SUBFOLDER_NAME "High Fidelity")
set(ESCAPED_DMG_SUBFOLDER_NAME "")
set(DMG_SUBFOLDER_ICON "${CMAKE_SOURCE_DIR}/cmake/installer/install-folder.rsrc")

set(CPACK_GENERATOR "DragNDrop")
include(CPack)
