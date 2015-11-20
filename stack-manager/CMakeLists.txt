cmake_minimum_required(VERSION 2.8.11)

if (POLICY CMP0028)
  cmake_policy(SET CMP0028 OLD)
endif ()

set(TARGET_NAME "StackManager")

project(${TARGET_NAME})

set(CMAKE_AUTOMOC ON)

if (NOT QT_CMAKE_PREFIX_PATH)
  set(QT_CMAKE_PREFIX_PATH $ENV{QT_CMAKE_PREFIX_PATH})
endif ()

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${QT_CMAKE_PREFIX_PATH})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Svg REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5WebKitWidgets REQUIRED)
find_package(QuaZip REQUIRED)

if (WIN32)
  find_package(ZLIB REQUIRED)
endif ()

include_directories(
  ${QUAZIP_INCLUDE_DIRS}
  ${ZLIB_INCLUDE_DIRS}
  src
  src/ui
  ${PROJECT_BINARY_DIR}/includes
)

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

file(GLOB SRCS "src/*.cpp" "src/ui/*.cpp")
file(GLOB HEADERS "src/*.h" "src/ui/*.h" "${PROJECT_BINARY_DIR}/includes/*.h")
file(GLOB QT_RES_FILES "src/*.qrc")
qt5_add_resources(QT_RES "${QT_RES_FILES}")
set(SM_SRCS ${QT_RES} ${SRCS} ${HEADERS})

if (APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 10.8)
  set(MACOSX_BUNDLE_BUNDLE_NAME "Stack Manager")
  set(MACOSX_BUNDLE_GUI_IDENTIFIER io.highfidelity.StackManager)
  set(MACOSX_BUNDLE_ICON_FILE icon.icns)
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  set(SM_SRCS ${SM_SRCS} "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.icns")
  add_executable(${TARGET_NAME} MACOSX_BUNDLE ${SM_SRCS})
else ()
  if (WIN32)
    add_executable(${TARGET_NAME} WIN32 ${SM_SRCS} windows_icon.rc)
  else ()
    add_executable(${TARGET_NAME} ${SM_SRCS})
  endif ()
endif ()

target_link_libraries(${TARGET_NAME} Qt5::Core Qt5::Gui Qt5::Svg Qt5::Network Qt5::Widgets Qt5::WebKitWidgets ${QUAZIP_LIBRARIES} ${ZLIB_LIBRARIES})
