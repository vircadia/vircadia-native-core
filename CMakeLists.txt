cmake_minimum_required(VERSION 2.8.12.2)

if (WIN32)
  cmake_policy(SET CMP0020 NEW)
endif (WIN32)

if (POLICY CMP0028)
  cmake_policy(SET CMP0028 OLD)
endif ()

if (POLICY CMP0043)
  cmake_policy(SET CMP0043 OLD)
endif () 

project(hifi)
add_definitions(-DGLM_FORCE_RADIANS)

if (WIN32)
  add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
  # set path for Microsoft SDKs
  # if get build error about missing 'glu32' this path is likely wrong
  # Uncomment the line with 8.1 if running Windows 8.1
  if (MSVC10)
    set(WINDOW_SDK_PATH "C:\\Program Files\\Microsoft SDKs\\Windows\\v7.1 ")
  elseif (MSVC12)
    set(WINDOW_SDK_PATH "C:\\Program Files (x86)\\Windows Kits\\8.1\\Lib\\winv6.3\\um\\x86 ")
  endif ()
  message (WINDOW_SDK_PATH= ${WINDOW_SDK_PATH})
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${WINDOW_SDK_PATH})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
elseif (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
  #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unknown-pragmas")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fno-strict-aliasing")
endif(WIN32)

if (ANDROID)
  if (NOT ANDROID_QT_CMAKE_PREFIX_PATH)
    set(QT_CMAKE_PREFIX_PATH $ENV{HIFI_ANDROID}/qt/5.3/android_armv7/lib/cmake)
  endif ()
elseif ()
  if (NOT QT_CMAKE_PREFIX_PATH)
    set(QT_CMAKE_PREFIX_PATH $ENV{QT_CMAKE_PREFIX_PATH})
  endif ()
endif ()

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${QT_CMAKE_PREFIX_PATH})

# set our OS X deployment target to
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.8)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

set(HIFI_LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libraries")

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")

set(MACRO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macros")

file(GLOB HIFI_CUSTOM_MACROS "cmake/macros/*.cmake")
foreach(CUSTOM_MACRO ${HIFI_CUSTOM_MACROS})
  include(${CUSTOM_MACRO})
endforeach()

# add subdirectories for all targets
add_subdirectory(assignment-client)

if (NOT ANDROID)
add_subdirectory(domain-server)
add_subdirectory(ice-server)
add_subdirectory(interface)
add_subdirectory(tests)
add_subdirectory(tools)
add_subdirectory(voxel-edit)
endif ()
