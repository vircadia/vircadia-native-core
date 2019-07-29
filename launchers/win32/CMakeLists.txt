cmake_minimum_required(VERSION 3.9.2)

project(HQLauncher)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release"
    CACHE STRING "Configuration types" FORCE)

# Use of MFC
set(CMAKE_MFC_FLAG 1)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
include("cmake/macros/SetPackagingParameters.cmake")

add_executable(HQLauncher
  WIN32
  libs/json/json.h
  libs/json/json-forwards.h
  libs/jsoncpp.cpp
  LauncherApp.cpp
  LauncherApp.h
  Launcher.rc
  LauncherDlg.cpp
  LauncherDlg.h
  LauncherManager.cpp
  LauncherManager.h
  LauncherUtils.cpp
  LauncherUtils.h
  libs/miniz.cpp
  libs/miniz.h
  res/HiFi_Logo_Large.png
  res/HiFi_Logo_Small.png
  res/HiFi_Voxel.png
  res/HiFi_Window.png
  res/interface.ico
  res/Launcher.rc2
  Resource.h
  stdafx.cpp
  stdafx.h
  targetver.h
)

function(set_from_env _RESULT_NAME _ENV_VAR_NAME _DEFAULT_VALUE)
    if (NOT DEFINED ${_RESULT_NAME}) 
        if ("$ENV{${_ENV_VAR_NAME}}" STREQUAL "")
            set (${_RESULT_NAME} ${_DEFAULT_VALUE} PARENT_SCOPE)
        else()
            set (${_RESULT_NAME} $ENV{${_ENV_VAR_NAME}} PARENT_SCOPE)
        endif()
    endif()
endfunction()

set_packaging_parameters()

set_from_env(LAUNCHER_HMAC_SECRET LAUNCHER_HMAC_SECRET "")

if (LAUNCHER_HMAC_SECRET STREQUAL "")
    message(FATAL_ERROR "LAUNCHER_HMAC_SECRET is not set")
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE LAUNCHER_HMAC_SECRET="${LAUNCHER_HMAC_SECRET}")
target_compile_definitions(${PROJECT_NAME} PRIVATE LAUNCHER_BUILD_VERSION="${BUILD_VERSION}")

# Preprocessor definitions
target_compile_definitions(HQLauncher PRIVATE
  $<$<CONFIG:Debug>:_UNICODE;_WINDOWS;_DEBUG>
  $<$<CONFIG:Release>:_UNICODE;_WINDOWS;NDEBUG>
)

# Minimal rebuild
if (MSVC)
  target_compile_options(HQLauncher PRIVATE
    "$<$<CONFIG:Debug>:/Gm->"
    "$<$<CONFIG:Release>:/Gm->"
  )
endif ()

# Precompiled header files
if (MSVC)
  target_compile_options(HQLauncher PRIVATE
    "$<$<CONFIG:Debug>:/Yu>"
    "$<$<CONFIG:Release>:/Yu>"
  )
  set_property(SOURCE stdafx.cpp
    APPEND_STRING PROPERTY COMPILE_FLAGS
    "$<$<CONFIG:Debug>:/Yc> \
     $<$<CONFIG:Release>:/Yc>")
endif ()
