# 
#  Created by Bradley Austin Davis on 2017/11/28
#  Copyright 2013-2017 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_POLYVOX)
    if (ANDROID)
        set(INSTALL_DIR ${HIFI_ANDROID_PRECOMPILED}/polyvox)
        set(POLYVOX_INCLUDE_DIRS "${INSTALL_DIR}/include" CACHE STRING INTERNAL)
        set(LIB_DIR ${INSTALL_DIR}/lib)
        list(APPEND POLYVOX_LIBRARIES ${LIB_DIR}/libPolyVoxUtil.so)
        list(APPEND POLYVOX_LIBRARIES ${LIB_DIR}/Release/libPolyVoxCore.so)
    else()
        find_library(POLYVOX_LIBRARY_RELEASE PolyVoxCore PATHS ${VCPKG_INSTALL_ROOT}/lib NO_DEFAULT_PATH)
        find_library(POLYVOX_UTIL_LIBRARY_RELEASE PolyVoxUtil PATHS ${VCPKG_INSTALL_ROOT}/lib NO_DEFAULT_PATH)
        list(APPEND POLYVOX_LIBRARY_RELEASE ${POLYVOX_UTIL_LIBRARY_RELEASE})
        find_library(POLYVOX_LIBRARY_DEBUG PolyVoxCore PATHS ${VCPKG_INSTALL_ROOT}/debug/lib NO_DEFAULT_PATH)
        find_library(POLYVOX_UTIL_LIBRARY_DEBUG PolyVoxUtil PATHS ${VCPKG_INSTALL_ROOT}/debug/lib NO_DEFAULT_PATH)
        list(APPEND POLYVOX_LIBRARY_DEBUG ${POLYVOX_UTIL_LIBRARY_DEBUG})
        select_library_configurations(POLYVOX)
        list(APPEND POLYVOX_INCLUDE_DIRS ${VCPKG_INSTALL_ROOT}/include)
    endif()
    target_link_libraries(${TARGET_NAME} ${POLYVOX_LIBRARIES})
    target_include_directories(${TARGET_NAME} PUBLIC ${POLYVOX_INCLUDE_DIRS})
endmacro()

