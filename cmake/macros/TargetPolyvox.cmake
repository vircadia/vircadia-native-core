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
        add_dependency_external_projects(polyvox)
        find_package(PolyVox REQUIRED)
    endif()
    target_include_directories(${TARGET_NAME} SYSTEM PUBLIC ${POLYVOX_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${POLYVOX_LIBRARIES})
endmacro()



