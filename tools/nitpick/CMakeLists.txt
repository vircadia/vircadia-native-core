set(TARGET_NAME nitpick)
project(${TARGET_NAME})

function(JOIN VALUES GLUE OUTPUT)
  string (REGEX REPLACE "([^\\]|^);" "\\1${GLUE}" _TMP_STR "${VALUES}")
  string (REGEX REPLACE "[\\](.)" "\\1" _TMP_STR "${_TMP_STR}") #fixes escaping
  set (${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
endfunction()

set(CUSTOM_NITPICK_QRC_PATHS "")

find_npm()

set(RESOURCES_QRC ${CMAKE_CURRENT_BINARY_DIR}/resources.qrc)
set(RESOURCES_RCC ${CMAKE_CURRENT_SOURCE_DIR}/compiledResources/resources.rcc)
generate_qrc(OUTPUT ${RESOURCES_QRC} PATH ${CMAKE_CURRENT_SOURCE_DIR}/resources CUSTOM_PATHS ${CUSTOM_NITPICK_QRC_PATHS} GLOBS *)

add_custom_command(
  OUTPUT ${RESOURCES_RCC}
  DEPENDS ${RESOURCES_QRC} ${GENERATE_QRC_DEPENDS}
  COMMAND "${QT_DIR}/bin/rcc"
  ARGS ${RESOURCES_QRC} -binary -o ${RESOURCES_RCC}
)

if (WIN32)
  set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} -bigobj")
  add_definitions(-D_USE_MATH_DEFINES) # apparently needed to get M_PI and other defines from cmath/math.h
  add_definitions(-DWINDOWS_LEAN_AND_MEAN) # needed to make sure windows doesn't go to crazy with its defines
endif()

# grab the implementation and header files from src dirs
file(GLOB_RECURSE NITPICK_SRCS "src/*.cpp" "src/*.h")
GroupSources("src")
list(APPEND NITPICK_SRCS ${RESOURCES_RCC})

find_package(
  Qt5 COMPONENTS
  Gui Widgets Multimedia Network Qml Quick Script Svg Xml
  ${PLATFORM_QT_COMPONENTS}
)

# grab the ui files in ui
file (GLOB_RECURSE QT_UI_FILES ui/*.ui)
source_group("UI Files" FILES ${QT_UI_FILES})

# have qt5 wrap them and generate the appropriate header files
qt5_wrap_ui(QT_UI_HEADERS "${QT_UI_FILES}")

# add them to the nitpick source files
set(NITPICK_SRCS ${NITPICK_SRCS} "${QT_UI_HEADERS}" "${QT_RESOURCES}")

if (APPLE)
  # configure CMake to use a custom Info.plist
  set_target_properties(${this_target} PROPERTIES MACOSX_BUNDLE_INFO_PLIST MacOSXBundleInfo.plist.in)

  if (PRODUCTION_BUILD)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER com.highfidelity.nitpick)
  else ()
    if (DEV_BUILD)
      set(MACOSX_BUNDLE_GUI_IDENTIFIER com.highfidelity.nitpick-dev)
    elseif (PR_BUILD)
      set(MACOSX_BUNDLE_GUI_IDENTIFIER com.highfidelity.nitpick-pr)
    endif ()
  endif ()

  # set how the icon shows up in the Info.plist file
  set(MACOSX_BUNDLE_ICON_FILE "${NITPICK_ICON_FILENAME}")

  # set where in the bundle to put the resources file
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/icon/${NITPICK_ICON_FILENAME} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

  # append the discovered resources to our list of nitpick sources
  list(APPEND NITPICK_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/icon/${NITPICK_ICON_FILENAME})
endif()

# create the executable, make it a bundle on OS X
if (APPLE)
  add_executable(${TARGET_NAME} MACOSX_BUNDLE ${NITPICK_SRCS} ${QM})

  # make sure the output name for the .app bundle is correct
  # Fix up the rpath so macdeployqt works
  set_target_properties(${TARGET_NAME} PROPERTIES INSTALL_RPATH "@executable_path/../Frameworks")
elseif (WIN32)
  # configure an rc file for the chosen icon
  set(CONFIGURE_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/icon/${NITPICK_ICON_FILENAME}")
  set(CONFIGURE_ICON_RC_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Icon.rc")
  configure_file("${HF_CMAKE_DIR}/templates/Icon.rc.in" ${CONFIGURE_ICON_RC_OUTPUT})

  set(APP_FULL_NAME "High Fidelity Nitpick")
  set(CONFIGURE_VERSION_INFO_RC_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/VersionInfo.rc")
  configure_file("${HF_CMAKE_DIR}/templates/VersionInfo.rc.in" ${CONFIGURE_VERSION_INFO_RC_OUTPUT})

  # add an executable that also has the icon itself and the configured rc file as resources
  add_executable(${TARGET_NAME} WIN32 ${NITPICK_SRCS} ${QM} ${CONFIGURE_ICON_RC_OUTPUT} ${CONFIGURE_VERSION_INFO_RC_OUTPUT})

##  if (NOT DEV_BUILD)
##    add_custom_command(
##      TARGET ${TARGET_NAME}
##      POST_BUILD
##      COMMAND "mt.exe" -manifest "${CMAKE_CURRENT_SOURCE_DIR}/nitpick.exe.manifest" -inputresource:"$<TARGET_FILE:${TARGET_NAME}>"\;\#1 -outputresource:"$<TARGET_FILE:${TARGET_NAME}>"\;\#1
##      COMMENT "Adding OS version support manifest to exe"
##    )
##  endif()
else ()
  add_executable(${TARGET_NAME} ${NITPICK_SRCS} ${QM})
endif ()

add_dependencies(${TARGET_NAME} resources)

# disable /OPT:REF and /OPT:ICF for the Debug builds
# This will prevent the following linker warnings
# LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification
if (WIN32)
  set_property(TARGET ${TARGET_NAME} APPEND_STRING PROPERTY LINK_FLAGS_DEBUG "/OPT:NOREF /OPT:NOICF")
endif()

link_hifi_libraries(entities-renderer)

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

# include headers for nitpick and NitpickConfig.
include_directories("${PROJECT_SOURCE_DIR}/src")

if (UNIX AND NOT ANDROID)
    if (CMAKE_SYSTEM_NAME MATCHES "Linux")
        # Linux
        target_link_libraries(${TARGET_NAME} pthread atomic)
    else ()
        # OSX
        target_link_libraries(${TARGET_NAME} pthread)
    endif ()
endif()

# assume we are using a Qt build without bearer management
add_definitions(-DQT_NO_BEARERMANAGEMENT)

if (APPLE)
  # link in required OS X frameworks and include the right GL headers
  find_library(OpenGL OpenGL)
  find_library(AppKit AppKit)

  target_link_libraries(${TARGET_NAME} ${OpenGL} ${AppKit})

  # setup install of OS X nitpick bundle
  install(TARGETS ${TARGET_NAME}
    BUNDLE DESTINATION ${NITPICK_INSTALL_DIR}
    COMPONENT ${CLIENT_COMPONENT}
  )

  # call the fixup_nitpick macro to add required bundling commands for installation
  fixup_nitpick()

else()
  set(NITPICK_EXEC_DIR "$<TARGET_FILE_DIR:${TARGET_NAME}>")
  set(RESOURCES_DEV_DIR "${NITPICK_EXEC_DIR}/resources")

  # copy the resources files beside the executable
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${RESOURCES_RCC}"
      "${NITPICK_EXEC_DIR}"
    # FIXME, the edit script code loads HTML from the scripts folder
    # which in turn relies on CSS that refers to the fonts.  In theory
    # we should be able to modify the CSS to reference the QRC path to
    # the ttf files, but doing so generates a CORS policy violation,
    # so we have to retain a copy of the fonts outside of the resources binary
  )

  # link target to external libraries
  if (WIN32)
    # setup install of executable and things copied by fixup/windeployqt
    install(
      DIRECTORY "$<TARGET_FILE_DIR:${TARGET_NAME}>/"
      DESTINATION ${NITPICK_INSTALL_DIR}
      COMPONENT ${CLIENT_COMPONENT}
      PATTERN "*.pdb" EXCLUDE
      PATTERN "*.lib" EXCLUDE
      PATTERN "*.exp" EXCLUDE
    )

    set(SCRIPTS_INSTALL_DIR "${NITPICK_INSTALL_DIR}")

    set(EXECUTABLE_COMPONENT ${CLIENT_COMPONENT})

    optional_win_executable_signing()
  endif()
endif()

if (WIN32)
    set(EXTRA_DEPLOY_OPTIONS "--qmldir  \"${PROJECT_SOURCE_DIR}/resources/qml\"")

    set(TARGET_INSTALL_DIR ${NITPICK_INSTALL_DIR})
    set(TARGET_INSTALL_COMPONENT ${CLIENT_COMPONENT})
    manually_install_openssl_for_qt()

    package_libraries_for_deployment()
endif()

# tell CMake to exclude ui_console.h for policy CMP0071
set_property(SOURCE ui_console.h PROPERTY SKIP_AUTOMOC ON)
