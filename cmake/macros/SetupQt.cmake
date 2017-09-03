# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

function(set_from_env _RESULT_NAME _ENV_VAR_NAME _DEFAULT_VALUE)
    if ("$ENV{${_ENV_VAR_NAME}}" STREQUAL "")
        set (${_RESULT_NAME} ${_DEFAULT_VALUE} PARENT_SCOPE)
    else()
        set (${_RESULT_NAME} $ENV{${_ENV_VAR_NAME}} PARENT_SCOPE)
    endif()
endfunction()

# Construct a default QT location from a root path, a version and an architecture
function(calculate_default_qt_dir _RESULT_NAME)
    if (ANDROID)
        set(QT_DEFAULT_ARCH "android_armv7")
    elseif(UWP)
        set(QT_DEFAULT_ARCH "winrt_x64_msvc2017")
    elseif(APPLE)
        set(QT_DEFAULT_ARCH "clang_64")
    elseif(WIN32)
        set(QT_DEFAULT_ARCH "msvc2017_64")
    else()
        set(QT_DEFAULT_ARCH "gcc_64")
    endif()

    if (WIN32)
        set(QT_DEFAULT_ROOT "c:/Qt")
    else()
        set(QT_DEFAULT_ROOT "$ENV{HOME}/Qt")
    endif()

    set_from_env(QT_ROOT QT_ROOT ${QT_DEFAULT_ROOT})
    set_from_env(QT_VERSION QT_VERSION "5.9.1")
    set_from_env(QT_ARCH QT_ARCH ${QT_DEFAULT_ARCH})

    set(${_RESULT_NAME} "${QT_ROOT}/${QT_VERSION}/${QT_ARCH}" PARENT_SCOPE)
endfunction()

# Sets the QT_CMAKE_PREFIX_PATH and QT_DIR variables
# Also enables CMAKE_AUTOMOC and CMAKE_AUTORCC
macro(setup_qt)
    set(QT_CMAKE_PREFIX_PATH "$ENV{QT_CMAKE_PREFIX_PATH}")
    if (("QT_CMAKE_PREFIX_PATH" STREQUAL "") OR (NOT EXISTS "${QT_CMAKE_PREFIX_PATH}"))
        calculate_default_qt_dir(QT_DIR)
        set(QT_CMAKE_PREFIX_PATH "${QT_DIR}/lib/cmake")
    else()
        # figure out where the qt dir is
        get_filename_component(QT_DIR "${QT_CMAKE_PREFIX_PATH}/../../" ABSOLUTE)
    endif()

    if (WIN32)
        # windows shell does not like backslashes expanded on the command line,
        # so convert all backslashes in the QT path to forward slashes
        string(REPLACE \\ / QT_CMAKE_PREFIX_PATH ${QT_CMAKE_PREFIX_PATH})
        string(REPLACE \\ / QT_DIR ${QT_DIR})
    endif()

    # This check doesn't work on Mac
    #if (NOT EXISTS "${QT_DIR}/include/QtCore/QtGlobal")
    #    message(FATAL_ERROR "Unable to locate Qt includes in ${QT_DIR}")
    #endif()
    
    if (NOT EXISTS "${QT_CMAKE_PREFIX_PATH}/Qt5Core/Qt5CoreConfig.cmake")
        message(FATAL_ERROR "Unable to locate Qt cmake config in ${QT_CMAKE_PREFIX_PATH}")
    endif()
    
    message(STATUS "The Qt build in use is: \"${QT_DIR}\"")

    # Instruct CMake to run moc automatically when needed.
    set(CMAKE_AUTOMOC ON)

    # Instruct CMake to run rcc automatically when needed
    set(CMAKE_AUTORCC ON)
    
    if (WIN32)
        add_paths_to_fixup_libs("${QT_DIR}/bin")
    endif ()

endmacro()