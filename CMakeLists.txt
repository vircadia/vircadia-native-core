cmake_minimum_required(VERSION 2.8.12.2)

if (USE_ANDROID_TOOLCHAIN)
  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/android/android.toolchain.cmake")
  set(ANDROID_NATIVE_API_LEVEL 19)
endif ()

if (WIN32)
  cmake_policy(SET CMP0020 NEW)
endif (WIN32)

if (POLICY CMP0028)
  cmake_policy(SET CMP0028 OLD)
endif ()

if (POLICY CMP0043)
  cmake_policy(SET CMP0043 OLD)
endif () 

if (POLICY CMP0042)
  cmake_policy(SET CMP0042 OLD)
endif ()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")

project(hifi)
add_definitions(-DGLM_FORCE_RADIANS)
set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG") 

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
  # /wd4351 disables warning C4351: new behavior: elements of array will be default initialized
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /wd4351")
  # /LARGEADDRESSAWARE enables 32-bit apps to use more than 2GB of memory.
  # Caveats: http://stackoverflow.com/questions/2288728/drawbacks-of-using-largeaddressaware-for-32-bit-windows-executables
  # TODO: Remove when building 64-bit.
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
else ()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fno-strict-aliasing -Wno-unused-parameter")
  if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -Woverloaded-virtual -Wdouble-promotion")
  endif ()
endif(WIN32)

if ((NOT MSVC12) AND (NOT MSVC14))
  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
  CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

  if (COMPILER_SUPPORTS_CXX11)
  	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  elseif(COMPILER_SUPPORTS_CXX0X)
  	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
  else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
  endif()
endif ()

if (APPLE) 
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --stdlib=libc++")
endif ()

if (NOT ANDROID_LIB_DIR)
  set(ANDROID_LIB_DIR $ENV{ANDROID_LIB_DIR})
endif ()

if (ANDROID)  
  if (NOT ANDROID_QT_CMAKE_PREFIX_PATH)
    set(QT_CMAKE_PREFIX_PATH ${ANDROID_LIB_DIR}/Qt/5.4/android_armv7/lib/cmake)
  else ()
    set(QT_CMAKE_PREFIX_PATH ${ANDROID_QT_CMAKE_PREFIX_PATH})
  endif ()
  
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
  
  if (ANDROID_LIB_DIR)
    list(APPEND CMAKE_FIND_ROOT_PATH ${ANDROID_LIB_DIR})
  endif ()
else ()
  if (NOT QT_CMAKE_PREFIX_PATH)
    set(QT_CMAKE_PREFIX_PATH $ENV{QT_CMAKE_PREFIX_PATH})
  endif ()
  if (NOT QT_CMAKE_PREFIX_PATH)
    get_filename_component(QT_CMAKE_PREFIX_PATH "${Qt5_DIR}/.." REALPATH)
  endif ()
endif ()

if (WIN32)
    if (NOT EXISTS ${QT_CMAKE_PREFIX_PATH})
        message(FATAL_ERROR "Could not determine QT_CMAKE_PREFIX_PATH.")
    endif ()
endif()

# figure out where the qt dir is
get_filename_component(QT_DIR "${QT_CMAKE_PREFIX_PATH}/../../" ABSOLUTE)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${QT_CMAKE_PREFIX_PATH})

if (APPLE)
  
  SET(OSX_SDK "10.9" CACHE String "OS X SDK version to look for inside Xcode bundle or at OSX_SDK_PATH")
  
  # set our OS X deployment target
  set(CMAKE_OSX_DEPLOYMENT_TARGET 10.8)
  
  # find the SDK path for the desired SDK
  find_path(
    _OSX_DESIRED_SDK_PATH 
    NAME MacOSX${OSX_SDK}.sdk 
    HINTS ${OSX_SDK_PATH}
    PATHS /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/
          /Applications/Xcode-beta.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/
  )
  
  if (NOT _OSX_DESIRED_SDK_PATH)
    message(FATAL_ERROR "Could not find OS X ${OSX_SDK} SDK. Please pass OSX_SDK_PATH to CMake to point us to your SDKs directory.")
  else ()
    message(STATUS "Found OS X ${OSX_SDK} SDK at ${_OSX_DESIRED_SDK_PATH}/MacOSX${OSX_SDK}.sdk")
  endif ()
  
  # set that as the SDK to use
  set(CMAKE_OSX_SYSROOT ${_OSX_DESIRED_SDK_PATH}/MacOSX${OSX_SDK}.sdk)
endif ()

# Hide automoc folders (for IDEs)
set(AUTOGEN_TARGETS_FOLDER "hidden/generated")

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
# Instruct CMake to run rcc automatically when needed
set(CMAKE_AUTORCC ON)

set(HIFI_LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libraries")

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")

if (CMAKE_BUILD_TYPE)
  string(TOUPPER ${CMAKE_BUILD_TYPE} UPPER_CMAKE_BUILD_TYPE)
else ()
  set(UPPER_CMAKE_BUILD_TYPE DEBUG)
endif ()

set(HIFI_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(MACRO_DIR "${HIFI_CMAKE_DIR}/macros")
set(EXTERNAL_PROJECT_DIR "${HIFI_CMAKE_DIR}/externals")

file(GLOB HIFI_CUSTOM_MACROS "cmake/macros/*.cmake")
foreach(CUSTOM_MACRO ${HIFI_CUSTOM_MACROS})
  include(${CUSTOM_MACRO})
endforeach()

if (ANDROID)
  file(GLOB ANDROID_CUSTOM_MACROS "cmake/android/*.cmake")
  foreach(CUSTOM_MACRO ${ANDROID_CUSTOM_MACROS})
    include(${CUSTOM_MACRO})
  endforeach()
endif ()

set(EXTERNAL_PROJECT_PREFIX "project")
set_property(DIRECTORY PROPERTY EP_PREFIX ${EXTERNAL_PROJECT_PREFIX})
setup_externals_binary_dir()

# setup options to grab external project dependencies
option(GET_BULLET "Get Bullet library automatically as external project" 1)
option(GET_GLM "Get GLM library automatically as external project" 1)
option(GET_GVERB "Get Gverb library automatically as external project" 1)
option(GET_SOXR "Get Soxr library automatically as external project" 1)
option(GET_TBB "Get Threading Building Blocks library automatically as external project" 1)
option(GET_LIBOVR "Get LibOVR library automatically as external project" 1)
option(GET_VHACD "Get V-HACD library automatically as external project" 1)
option(GET_POLYVOX "Get polyvox library automatically as external project" 1)
option(GET_OPENVR "Get OpenVR library automatically as external project" 1)
option(GET_BOOSTCONFIG "Get Boost-config library automatically as external project" 1)
option(GET_OGLPLUS "Get OGLplus library automatically as external project" 1)
option(GET_GLEW "Get GLEW library automatically as external project" 1)

option(USE_NSIGHT "Attempt to find the nSight libraries" 1)

option(GET_SDL2 "Get SDL2 library automatically as external project" 0)

if (WIN32)
  add_paths_to_fixup_libs("${QT_DIR}/bin")
endif ()

# add subdirectories for all targets
if (NOT ANDROID)
  add_subdirectory(assignment-client)
  set_target_properties(assignment-client PROPERTIES FOLDER "Apps")
  add_subdirectory(domain-server)
  set_target_properties(domain-server PROPERTIES FOLDER "Apps")
  add_subdirectory(ice-server)
  set_target_properties(ice-server PROPERTIES FOLDER "Apps")
  add_subdirectory(interface)
  set_target_properties(interface PROPERTIES FOLDER "Apps")
  add_subdirectory(tests)
  add_subdirectory(tools)
endif ()

if (ANDROID OR DESKTOP_GVR)
  add_subdirectory(gvr-interface)
endif ()

if (DEFINED ENV{HIFI_MEMORY_DEBUGGING})
  SET( HIFI_MEMORY_DEBUGGING true )
endif ()
if (HIFI_MEMORY_DEBUGGING)
  if (UNIX)
    MESSAGE("-- Memory debugging is enabled")
  endif (UNIX)
endif ()
