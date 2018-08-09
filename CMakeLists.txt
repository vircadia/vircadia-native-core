# If we're running under the gradle build, HIFI_ANDROID will be set here, but
# ANDROID will not be set until after the `project` statement.  This is the *ONLY*
# place you need to use `HIFI_ANDROID` instead of `ANDROID`
if (WIN32 AND NOT HIFI_ANDROID)
  cmake_minimum_required(VERSION 3.7)
else()
  cmake_minimum_required(VERSION 3.2)
endif()

project(hifi)

include("cmake/init.cmake")

include("cmake/compiler.cmake")

if (BUILD_SCRIBE_ONLY)
  add_subdirectory(tools/scribe)
  add_subdirectory(tools/shader_reflect)
  return()
endif()

if (NOT DEFINED CLIENT_ONLY)
  set(CLIENT_ONLY 0)
endif()

if (NOT DEFINED SERVER_ONLY)
  set(SERVER_ONLY 0)
endif()

if (ANDROID OR UWP)
  set(MOBILE 1)
else()
  set(MOBILE 0)
endif()

set(BUILD_CLIENT_OPTION ON)
set(BUILD_SERVER_OPTION ON)
set(BUILD_TESTS_OPTION ON)
set(BUILD_TOOLS_OPTION ON)
set(BUILD_INSTALLER_OPTION ON)
set(GLES_OPTION OFF)
set(DISABLE_QML_OPTION OFF)
set(DOWNLOAD_SERVERLESS_CONTENT_OPTION OFF)

if (ANDROID OR UWP)
  set(BUILD_SERVER_OPTION OFF)
  set(BUILD_TOOLS_OPTION OFF)
  set(BUILD_INSTALLER OFF)
endif()

if (CLIENT_ONLY)
  set(BUILD_SERVER_OPTION OFF)
endif()

if (SERVER_ONLY)
  set(BUILD_CLIENT_OPTION OFF)
  set(BUILD_TESTS_OPTION OFF)
endif()

if (ANDROID)
  set(GLES_OPTION ON)
  set(PLATFORM_QT_COMPONENTS AndroidExtras WebView)
else ()
  set(PLATFORM_QT_COMPONENTS WebEngine WebEngineWidgets)
endif ()

if (USE_GLES AND (NOT ANDROID))
  set(DISABLE_QML_OPTION ON)
endif()

option(BUILD_CLIENT "Build client components" ${BUILD_CLIENT_OPTION})
option(BUILD_SERVER "Build server components" ${BUILD_SERVER_OPTION})
option(BUILD_TESTS "Build tests" ${BUILD_TESTS_OPTION})
option(BUILD_TOOLS "Build tools" ${BUILD_TOOLS_OPTION})
option(BUILD_INSTALLER "Build installer" ${BUILD_INSTALLER_OPTION})
option(USE_GLES "Use OpenGL ES" ${GLES_OPTION})
option(DISABLE_QML "Disable QML" ${DISABLE_QML_OPTION})
option(DISABLE_KTX_CACHE "Disable KTX Cache" OFF)
option(
  DOWNLOAD_SERVERLESS_CONTENT
  "Download and setup default serverless content beside Interface"
  ${DOWNLOAD_SERVERLESS_CONTENT_OPTION}
)

set(PLATFORM_QT_GL OpenGL)

if (USE_GLES)
  add_definitions(-DUSE_GLES)
  set(PLATFORM_GL_BACKEND gpu-gl-common gpu-gles)
else()
  set(PLATFORM_GL_BACKEND gpu-gl-common gpu-gl)
endif()

foreach(PLATFORM_QT_COMPONENT ${PLATFORM_QT_COMPONENTS})
  list(APPEND PLATFORM_QT_LIBRARIES "Qt5::${PLATFORM_QT_COMPONENT}")
endforeach()

MESSAGE(STATUS "Build server:          " ${BUILD_SERVER})
MESSAGE(STATUS "Build client:          " ${BUILD_CLIENT})
MESSAGE(STATUS "Build tests:           " ${BUILD_TESTS})
MESSAGE(STATUS "Build tools:           " ${BUILD_TOOLS})
MESSAGE(STATUS "Build installer:       " ${BUILD_INSTALLER})
MESSAGE(STATUS "GL ES:                 " ${USE_GLES})
MESSAGE(STATUS "DL serverless content: " ${DOWNLOAD_SERVERLESS_CONTENT})

if (DISABLE_QML)
  MESSAGE(STATUS "QML disabled!")
  add_definitions(-DDISABLE_QML)
endif()

if (DISABLE_KTX_CACHE)
  MESSAGE(STATUS "KTX cache disabled!")
  add_definitions(-DDISABLE_KTX_CACHE)
endif()

if (UNIX AND DEFINED ENV{HIFI_MEMORY_DEBUGGING})
  MESSAGE(STATUS "Memory debugging is enabled")
endif()

#
# Helper projects
#
file(GLOB_RECURSE CMAKE_SRC cmake/*.cmake cmake/CMakeLists.txt)
add_custom_target(cmake SOURCES ${CMAKE_SRC})
GroupSources("cmake")
unset(CMAKE_SRC)

file(GLOB_RECURSE JS_SRC scripts/*.js unpublishedScripts/*.js)
add_custom_target(js SOURCES ${JS_SRC})
GroupSources("scripts")
GroupSources("unpublishedScripts")
unset(JS_SRC)

# Locate the required Qt build on the filesystem
setup_qt()
list(APPEND CMAKE_PREFIX_PATH "${QT_CMAKE_PREFIX_PATH}")

find_package( Threads )

add_definitions(-DGLM_FORCE_RADIANS)
set(HIFI_LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libraries")

set(EXTERNAL_PROJECT_PREFIX "project")
set_property(DIRECTORY PROPERTY EP_PREFIX ${EXTERNAL_PROJECT_PREFIX})
setup_externals_binary_dir()

option(USE_NSIGHT "Attempt to find the nSight libraries" 1)

set_packaging_parameters()

# FIXME hack to work on the proper Android toolchain
if (ANDROID)
  add_subdirectory(android/app)
  return()
endif()

# add subdirectories for all targets
if (BUILD_SERVER)
  add_subdirectory(assignment-client)
  set_target_properties(assignment-client PROPERTIES FOLDER "Apps")
  add_subdirectory(domain-server)
  set_target_properties(domain-server PROPERTIES FOLDER "Apps")
  add_subdirectory(ice-server)
  set_target_properties(ice-server PROPERTIES FOLDER "Apps")
  add_subdirectory(server-console)
endif()

if (BUILD_CLIENT)
  add_subdirectory(interface)
  set_target_properties(interface PROPERTIES FOLDER "Apps")

  option(USE_SIXENSE "Build Interface with sixense library/plugin" OFF)
endif()

if (BUILD_CLIENT OR BUILD_SERVER)
  add_subdirectory(plugins)
endif()

# BUILD_TOOLS option will be handled inside the tools's CMakeLists.txt because 'scribe' tool is required for build anyway
add_subdirectory(tools)

if (BUILD_TESTS)
  # Turn on testing so that add_test works
  # MUST be in the root cmake file for ctest to work 
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
  add_subdirectory(tests-manual)
endif()

if (BUILD_INSTALLER)
  if (UNIX)
    install(
        DIRECTORY "${CMAKE_SOURCE_DIR}/scripts"
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/interface
        COMPONENT ${CLIENT_COMPONENT}
    )
  endif()
  generate_installers()
endif()
