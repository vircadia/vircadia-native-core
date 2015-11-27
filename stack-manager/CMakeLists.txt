set(TARGET_NAME "stack-manager")
set(BUILD_BUNDLE YES)
setup_hifi_project(Widgets Gui Svg Core Network WebKitWidgets)
add_dependencies(${TARGET_NAME} assignment-client domain-server)
include_application_version()

if (WIN32)
  target_zlib()
endif ()
target_quazip()

if (UNIX)
  set_target_properties(
    ${TARGET_NAME} PROPERTIES
    EXCLUDE_FROM_ALL TRUE
  )
endif (UNIX)

if (DEFINED ENV{JOB_ID})
  set(PR_BUILD "false")
  set(BUILD_SEQ $ENV{JOB_ID})
  set(BASE_URL "http://s3.amazonaws.com/hifi-public")
else ()
  set(BUILD_SEQ "dev")
  if (DEFINED ENV{PR_NUMBER})
    set(PR_BUILD "true")
    set(BASE_URL "http://s3.amazonaws.com/hifi-public/pr-builds/$ENV{PR_NUMBER}")
  else ()
    set(PR_BUILD "false")
    set(BASE_URL "http://s3.amazonaws.com/hifi-public")
  endif ()
endif ()

configure_file(src/StackManagerVersion.h.in "${PROJECT_BINARY_DIR}/includes/StackManagerVersion.h")
include_directories(
    ${PROJECT_BINARY_DIR}/includes
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/ui
    ${QUAZIP_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
)

if (APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 10.8)
  set(MACOSX_BUNDLE_BUNDLE_NAME "Stack Manager")
  set(MACOSX_BUNDLE_GUI_IDENTIFIER io.highfidelity.StackManager)
  set(MACOSX_BUNDLE_ICON_FILE icon.icns)
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  set(SM_SRCS ${SM_SRCS} "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.icns")
endif ()

package_libraries_for_deployment()
consolidate_stack_components()