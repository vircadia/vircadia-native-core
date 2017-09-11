if (WIN32)
  cmake_minimum_required(VERSION 3.7)
else()
  cmake_minimum_required(VERSION 3.2)
endif()

include("cmake/init.cmake")

project(hifi)

include("cmake/compiler.cmake")

if (NOT DEFINED SERVER_ONLY)
  set(SERVER_ONLY 0)
endif()

if (ANDROID OR UWP)
    set(MOBILE 1)
else()
    set(MOBILE 0)
endif()

if (ANDROID OR UWP)
    option(BUILD_SERVER "Build server components" OFF)
    option(BUILD_TOOLS "Build tools" OFF)
else()
    option(BUILD_SERVER "Build server components" ON)
    option(BUILD_TOOLS "Build tools" ON)
endif()

if (SERVER_ONLY)
    option(BUILD_CLIENT "Build client components" OFF)
    option(BUILD_TESTS "Build tests" OFF)
else()
    option(BUILD_CLIENT "Build client components" ON)
    option(BUILD_TESTS "Build tests" ON)
endif()

option(BUILD_INSTALLER "Build installer" ON)

MESSAGE(STATUS "Build server:    " ${BUILD_SERVER})
MESSAGE(STATUS "Build client:    " ${BUILD_CLIENT})
MESSAGE(STATUS "Build tests:     " ${BUILD_TESTS})
MESSAGE(STATUS "Build tools:     " ${BUILD_TOOLS})
MESSAGE(STATUS "Build installer: " ${BUILD_INSTALLER})

if (UNIX AND DEFINED ENV{HIFI_MEMORY_DEBUGGING})
    MESSAGE(STATUS "Memory debugging is enabled")
endif()

#
# Helper projects
#
file(GLOB_RECURSE CMAKE_SRC cmake/*.cmake cmake/CMakeLists.txt)
add_custom_target(cmake SOURCES ${CMAKE_SRC})
GroupSources("cmake")

file(GLOB_RECURSE JS_SRC scripts/*.js unpublishedScripts/*.js)
add_custom_target(js SOURCES ${JS_SRC})
GroupSources("scripts")
GroupSources("unpublishedScripts")

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
    if (ANDROID)
        add_subdirectory(gvr-interface)
        set_target_properties(gvr-interface PROPERTIES FOLDER "Apps")
    endif()
endif()

if (BUILD_CLIENT OR BUILD_SERVER)
    add_subdirectory(plugins)
endif()

if (BUILD_TOOLS)
  add_subdirectory(tools)
endif()

if (BUILD_TESTS)
    add_subdirectory(tests)
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
