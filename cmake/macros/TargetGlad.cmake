# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_GLAD)
    if (ANDROID)
        include(SelectLibraryConfigurations)
        set(INSTALL_DIR ${HIFI_ANDROID_PRECOMPILED}/glad)
        set(GLAD_INCLUDE_DIRS "${INSTALL_DIR}/include")
        set(GLAD_LIBRARY_DEBUG ${INSTALL_DIR}/lib/libglad_d.a)
        set(GLAD_LIBRARY_RELEASE ${INSTALL_DIR}/lib/libglad.a)
        select_library_configurations(GLAD)
        find_library(EGL EGL)
        target_link_libraries(${TARGET_NAME} ${EGL})
    else()
        find_package(OpenGL REQUIRED)
        list(APPEND GLAD_EXTRA_LIBRARIES ${OPENGL_LIBRARY})
        find_library(GLAD_LIBRARY_RELEASE glad PATHS ${VCPKG_INSTALL_ROOT}/lib NO_DEFAULT_PATH)
        find_library(GLAD_LIBRARY_DEBUG glad_d PATHS ${VCPKG_INSTALL_ROOT}/debug/lib NO_DEFAULT_PATH)
        select_library_configurations(GLAD)
        if (NOT WIN32)
            list(APPEND GLAD_EXTRA_LIBRARIES dl)
        endif() 
        set(GLAD_INCLUDE_DIRS ${${GLAD_UPPER}_INCLUDE_DIRS})
    endif()

    target_include_directories(${TARGET_NAME} PUBLIC ${GLAD_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${GLAD_LIBRARY})
    target_link_libraries(${TARGET_NAME} ${GLAD_EXTRA_LIBRARIES})       
endmacro()
