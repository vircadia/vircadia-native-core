set(TARGET_NAME interface)
project(${TARGET_NAME})

# set a default root dir for each of our optional externals if it was not passed
set(OPTIONAL_EXTERNALS "LeapMotion")

foreach(EXTERNAL ${OPTIONAL_EXTERNALS})
  string(TOUPPER ${EXTERNAL} ${EXTERNAL}_UPPERCASE)
  if (NOT ${${EXTERNAL}_UPPERCASE}_ROOT_DIR)
    string(TOLOWER ${EXTERNAL} ${EXTERNAL}_LOWERCASE)
    set(${${EXTERNAL}_UPPERCASE}_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/${${EXTERNAL}_LOWERCASE}")
  endif ()
endforeach()

find_package(Qt5LinguistTools REQUIRED)
find_package(Qt5LinguistToolsMacros)

if (WIN32)
  add_definitions(-D_USE_MATH_DEFINES) # apparently needed to get M_PI and other defines from cmath/math.h
  add_definitions(-DWINDOWS_LEAN_AND_MEAN) # needed to make sure windows doesn't go to crazy with its defines
endif()

# grab the implementation and header files from src dirs
file(GLOB_RECURSE INTERFACE_SRCS "src/*.cpp" "src/*.h")
GroupSources("src")

# Add SpeechRecognizer if on Windows or OS X, otherwise remove
if (WIN32)
  # Use .cpp and .h files as is.
elseif (APPLE)
  file(GLOB INTERFACE_OBJCPP_SRCS "src/SpeechRecognizer.mm")
  set(INTERFACE_SRCS ${INTERFACE_SRCS} ${INTERFACE_OBJCPP_SRCS})
  get_filename_component(SPEECHRECOGNIZER_CPP "src/SpeechRecognizer.cpp" ABSOLUTE)
  list(REMOVE_ITEM INTERFACE_SRCS ${SPEECHRECOGNIZER_CPP})
else ()
  get_filename_component(SPEECHRECOGNIZER_H "src/SpeechRecognizer.h" ABSOLUTE)
  list(REMOVE_ITEM INTERFACE_SRCS ${SPEECHRECOGNIZER_H})
  get_filename_component(SPEECHRECOGNIZER_CPP "src/SpeechRecognizer.cpp" ABSOLUTE)
  list(REMOVE_ITEM INTERFACE_SRCS ${SPEECHRECOGNIZER_CPP})
endif ()

if (ANDROID)
  set(PLATFORM_QT_COMPONENTS AndroidExtras)
else ()
  set(PLATFORM_QT_COMPONENTS WebEngine WebEngineWidgets WebKitWidgets)
endif ()

find_package(
  Qt5 COMPONENTS
  Gui Multimedia Network OpenGL Qml Quick Script ScriptTools Svg
  ${QT_WEBENGINE_COMPONENTS}
  WebChannel WebSockets
)

# grab the ui files in resources/ui
file (GLOB_RECURSE QT_UI_FILES ui/*.ui)
source_group("UI Files" FILES ${QT_UI_FILES})
# have qt5 wrap them and generate the appropriate header files
qt5_wrap_ui(QT_UI_HEADERS "${QT_UI_FILES}")

# add them to the interface source files
set(INTERFACE_SRCS ${INTERFACE_SRCS} "${QT_UI_HEADERS}" "${QT_RESOURCES}")

# translation disabled until we strip out the line numbers
# set(QM ${TARGET_NAME}_en.qm)
# set(TS ${TARGET_NAME}_en.ts)
# qt5_create_translation_custom(${QM} ${INTERFACE_SRCS} ${QT_UI_FILES} ${TS})

# setup the android parameters that will help us produce an APK
if (ANDROID)
  set(ANDROID_APK_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/apk-build")
  set(ANDROID_APK_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/apk")

  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ANDROID_APK_OUTPUT_DIR}/libs/${ANDROID_ABI}")

  set(ANDROID_SDK_ROOT $ENV{ANDROID_HOME})
  set(ANDROID_APP_DISPLAY_NAME Interface)
  set(ANDROID_API_LEVEL 19)
  set(ANDROID_APK_PACKAGE io.highfidelity.interface)
  set(ANDROID_ACTIVITY_NAME io.highfidelity.interface.InterfaceActivity)
  set(ANDROID_APK_VERSION_NAME "0.1")
  set(ANDROID_APK_VERSION_CODE 1)
  set(ANDROID_APK_FULLSCREEN TRUE)
  set(ANDROID_DEPLOY_QT_INSTALL "--install")

  set(BUILD_SHARED_LIBS ON)
endif ()

if (APPLE)

  # configure CMake to use a custom Info.plist
  set_target_properties(${this_target} PROPERTIES MACOSX_BUNDLE_INFO_PLIST MacOSXBundleInfo.plist.in)

  if (PRODUCTION_BUILD)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER com.highfidelity.interface)
  else ()
    if (DEV_BUILD)
      set(MACOSX_BUNDLE_GUI_IDENTIFIER com.highfidelity.interface-dev)
    elseif (PR_BUILD)
      set(MACOSX_BUNDLE_GUI_IDENTIFIER com.highfidelity.interface-pr)
    endif ()
  endif ()

  # set how the icon shows up in the Info.plist file
  set(MACOSX_BUNDLE_ICON_FILE "${INTERFACE_ICON_FILENAME}")

  # set where in the bundle to put the resources file
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/icon/${INTERFACE_ICON_FILENAME} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

  set(DISCOVERED_RESOURCES "")

  # use the add_resources_to_os_x_bundle macro to recurse into resources
  add_resources_to_os_x_bundle("${CMAKE_CURRENT_SOURCE_DIR}/resources")

  # append the discovered resources to our list of interface sources
  list(APPEND INTERFACE_SRCS ${DISCOVERED_RESOURCES})

  set(INTERFACE_SRCS ${INTERFACE_SRCS} "${CMAKE_CURRENT_SOURCE_DIR}/icon/${INTERFACE_ICON_FILENAME}")
endif()

# create the executable, make it a bundle on OS X
if (APPLE)
  add_executable(${TARGET_NAME} MACOSX_BUNDLE ${INTERFACE_SRCS} ${QM})

  # make sure the output name for the .app bundle is correct
  set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME ${INTERFACE_BUNDLE_NAME})
elseif (WIN32)
  # configure an rc file for the chosen icon
  set(CONFIGURE_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/icon/${INTERFACE_ICON_FILENAME}")
  set(CONFIGURE_ICON_RC_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Icon.rc")
  configure_file("${HF_CMAKE_DIR}/templates/Icon.rc.in" ${CONFIGURE_ICON_RC_OUTPUT})

  # add an executable that also has the icon itself and the configured rc file as resources
  add_executable(${TARGET_NAME} WIN32 ${INTERFACE_SRCS} ${QM} ${CONFIGURE_ICON_RC_OUTPUT})

  if (NOT DEV_BUILD)
    add_custom_command(
      TARGET ${TARGET_NAME}
      POST_BUILD
      COMMAND "mt.exe" -manifest "${CMAKE_CURRENT_SOURCE_DIR}/interface.exe.manifest" -inputresource:"$<TARGET_FILE:${TARGET_NAME}>"\;\#1 -outputresource:"$<TARGET_FILE:${TARGET_NAME}>"\;\#1
      COMMENT "Adding OS version support manifest to exe"
    )
  endif()

elseif (ANDROID)
  # on android the Interface target is a library that gets linked/used by the APK shell that qtcreateapk produces
  add_library(${TARGET_NAME} ${INTERFACE_SRCS} ${QM})
else ()
  add_executable(${TARGET_NAME} ${INTERFACE_SRCS} ${QM})
endif ()

target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_BINARY_DIR}/includes")

if (WIN32)
  # These are external plugins, but we need to do the 'add dependency' here so that their
  # binary directories get added to the fixup path
  add_dependency_external_projects(sixense)
  add_dependency_external_projects(sdl2)
  add_dependency_external_projects(OpenVR)
  add_dependency_external_projects(neuron)
endif()

# disable /OPT:REF and /OPT:ICF for the Debug builds
# This will prevent the following linker warnings
# LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification
if (WIN32)
  set_property(TARGET ${TARGET_NAME} APPEND_STRING PROPERTY LINK_FLAGS_DEBUG "/OPT:NOREF /OPT:NOICF")
endif()

if (NOT ANDROID)
  set(${STEAM_WRAPPER} steamworks-wrapper)
endif ()

# link required hifi libraries
link_hifi_libraries(
  shared octree gpu gl gpu-gl procedural model render
  recording fbx networking model-networking entities avatars
  audio audio-client animation script-engine physics
  render-utils entities-renderer ui auto-updater
  controllers plugins ui-plugins display-plugins input-plugins
  ${STEAM_WRAPPER}
)

# include the binary directory of render-utils for shader includes
target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_BINARY_DIR}/libraries/render-utils")

#fixme find a way to express faceshift as a plugin
target_bullet()
target_opengl()

if (NOT ANDROID)
  target_glew()
endif ()

if (WIN32 OR APPLE)
  target_faceshift()
endif()

# perform standard include and linking for found externals
foreach(EXTERNAL ${OPTIONAL_EXTERNALS})

  if (${${EXTERNAL}_UPPERCASE}_REQUIRED)
    find_package(${EXTERNAL} REQUIRED)
  else ()
    find_package(${EXTERNAL})
  endif ()

  if (${${EXTERNAL}_UPPERCASE}_FOUND AND NOT DISABLE_${${EXTERNAL}_UPPERCASE})
    add_definitions(-DHAVE_${${EXTERNAL}_UPPERCASE})

    # include the library directories (ignoring warnings)
    if (NOT ${${EXTERNAL}_UPPERCASE}_INCLUDE_DIRS)
      set(${${EXTERNAL}_UPPERCASE}_INCLUDE_DIRS ${${${EXTERNAL}_UPPERCASE}_INCLUDE_DIR})
    endif ()

    include_directories(SYSTEM ${${${EXTERNAL}_UPPERCASE}_INCLUDE_DIRS})

    # perform the system include hack for OS X to ignore warnings
    if (APPLE)
      foreach(EXTERNAL_INCLUDE_DIR  ${${${EXTERNAL}_UPPERCASE}_INCLUDE_DIRS})
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${EXTERNAL_INCLUDE_DIR}")
      endforeach()
    endif ()

    if (NOT ${${EXTERNAL}_UPPERCASE}_LIBRARIES)
      set(${${EXTERNAL}_UPPERCASE}_LIBRARIES ${${${EXTERNAL}_UPPERCASE}_LIBRARY})
    endif ()

    if (NOT APPLE OR NOT ${${EXTERNAL}_UPPERCASE} MATCHES "SIXENSE")
      target_link_libraries(${TARGET_NAME} ${${${EXTERNAL}_UPPERCASE}_LIBRARIES})
    elseif (APPLE AND NOT INSTALLER_BUILD)
      add_definitions(-DSIXENSE_LIB_FILENAME=\"${${${EXTERNAL}_UPPERCASE}_LIBRARY_RELEASE}\")
    endif ()
  endif ()
endforeach()

# include headers for interface and InterfaceConfig.
include_directories("${PROJECT_SOURCE_DIR}/src")

target_link_libraries(
  ${TARGET_NAME}
  Qt5::Gui Qt5::Network Qt5::Multimedia Qt5::OpenGL
  Qt5::Qml Qt5::Quick Qt5::Script Qt5::ScriptTools Qt5::Svg
  Qt5::WebChannel Qt5::WebEngine Qt5::WebEngineWidgets Qt5::WebKitWidgets
)

# Issue causes build failure unless we add this directory.
# See https://bugreports.qt.io/browse/QTBUG-43351
if (WIN32)
  add_paths_to_fixup_libs(${Qt5_DIR}/../../../plugins/qtwebengine)
endif()

if (UNIX)
    target_link_libraries(${TARGET_NAME} pthread)
endif(UNIX)

# assume we are using a Qt build without bearer management
add_definitions(-DQT_NO_BEARERMANAGEMENT)

if (APPLE)
  # link in required OS X frameworks and include the right GL headers
  find_library(OpenGL OpenGL)
  find_library(AppKit AppKit)

  target_link_libraries(${TARGET_NAME} ${OpenGL} ${AppKit})

  # setup install of OS X interface bundle
  install(TARGETS ${TARGET_NAME}
    BUNDLE DESTINATION ${INTERFACE_INSTALL_DIR}
    COMPONENT ${CLIENT_COMPONENT}
  )

  set(SCRIPTS_INSTALL_DIR "${INTERFACE_INSTALL_APP_PATH}/Contents/Resources")

  # copy script files beside the executable
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
    "${CMAKE_SOURCE_DIR}/scripts"
    $<TARGET_FILE_DIR:${TARGET_NAME}>/../Resources/scripts
  )

  # call the fixup_interface macro to add required bundling commands for installation
  fixup_interface()

else (APPLE)
  # copy the resources files beside the executable
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
    "${PROJECT_SOURCE_DIR}/resources"
    $<TARGET_FILE_DIR:${TARGET_NAME}>/resources
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
    "${CMAKE_SOURCE_DIR}/scripts"
    $<TARGET_FILE_DIR:${TARGET_NAME}>/scripts
  )

  # link target to external libraries
  if (WIN32)
    target_link_libraries(${TARGET_NAME} wsock32.lib Winmm.lib)

    # setup install of executable and things copied by fixup/windeployqt
    install(
      FILES "$<TARGET_FILE_DIR:${TARGET_NAME}>/"
      DESTINATION ${INTERFACE_INSTALL_DIR}
      COMPONENT ${CLIENT_COMPONENT}
    )

    set(SCRIPTS_INSTALL_DIR "${INTERFACE_INSTALL_DIR}")

    set(EXECUTABLE_COMPONENT ${CLIENT_COMPONENT})

    optional_win_executable_signing()
  endif()
endif (APPLE)

if (SCRIPTS_INSTALL_DIR)

  # setup install of scripts beside interface executable
  install(
    DIRECTORY "${CMAKE_SOURCE_DIR}/scripts/"
    DESTINATION ${SCRIPTS_INSTALL_DIR}/scripts
    COMPONENT ${CLIENT_COMPONENT}
  )
endif()

add_bugsplat()

if (WIN32)
    set(EXTRA_DEPLOY_OPTIONS "--qmldir  ${PROJECT_SOURCE_DIR}/resources/qml")

    set(TARGET_INSTALL_DIR ${INTERFACE_INSTALL_DIR})
    set(TARGET_INSTALL_COMPONENT ${CLIENT_COMPONENT})
    manually_install_openssl_for_qt()

    package_libraries_for_deployment()
endif()

if (ANDROID)
  set(HIFI_URL_INTENT "<intent-filter>\
    \n        <action android:name='android.intent.action.VIEW' />\
    \n        <category android:name='android.intent.category.DEFAULT' />\
    \n        <category android:name='android.intent.category.BROWSABLE' />\
    \n        <data android:scheme='hifi' />\
    \n      </intent-filter>"
  )

  set(ANDROID_EXTRA_ACTIVITY_XML "${HIFI_URL_INTENT}")

  qt_create_apk()
endif ()
