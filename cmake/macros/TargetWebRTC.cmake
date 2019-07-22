#
#  Copyright 2019 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#
macro(TARGET_WEBRTC)
    if (ANDROID)
        include(SelectLibraryConfigurations)
        set(INSTALL_DIR ${HIFI_ANDROID_PRECOMPILED}/webrtc/webrtc)
        set(WEBRTC_INCLUDE_DIRS "${INSTALL_DIR}/include/webrtc")
        set(WEBRTC_LIBRARY_DEBUG ${INSTALL_DIR}/debug/lib/libwebrtc.a)
        set(WEBRTC_LIBRARY_RELEASE ${INSTALL_DIR}/lib/libwebrtc.a)
        select_library_configurations(WEBRTC)
    else()
        set(WEBRTC_INCLUDE_DIRS "${VCPKG_INSTALL_ROOT}/include/webrtc")
        find_library(WEBRTC_LIBRARY NAMES webrtc PATHS ${VCPKG_INSTALL_ROOT}/lib/ NO_DEFAULT_PATH)
    endif()

    target_include_directories(${TARGET_NAME} SYSTEM PUBLIC ${WEBRTC_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${WEBRTC_LIBRARY})

endmacro()
