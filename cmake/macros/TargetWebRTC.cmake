#
#  Copyright 2019 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#
macro(TARGET_WEBRTC)
    if (ANDROID)
        # I don't yet have working libwebrtc for android
        # include(SelectLibraryConfigurations)
        # set(INSTALL_DIR ${HIFI_ANDROID_PRECOMPILED}/webrtc/webrtc)
        # set(WEBRTC_INCLUDE_DIRS "${INSTALL_DIR}/include/webrtc")
        # set(WEBRTC_LIBRARY_DEBUG ${INSTALL_DIR}/debug/lib/libwebrtc.a)
        # set(WEBRTC_LIBRARY_RELEASE ${INSTALL_DIR}/lib/libwebrtc.a)
        # select_library_configurations(WEBRTC)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        # WebRTC is basically impossible to build on aarch64 Linux.
        # I am looking at https://gitlab.freedesktop.org/pulseaudio/webrtc-audio-processing for an alternative.
    else()
        set(WEBRTC_INCLUDE_DIRS "${VCPKG_INSTALL_ROOT}/include/webrtc")
        target_include_directories(${TARGET_NAME} SYSTEM PUBLIC ${WEBRTC_INCLUDE_DIRS})
        find_library(WEBRTC_LIBRARY_RELEASE webrtc PATHS ${VCPKG_INSTALL_ROOT}/lib NO_DEFAULT_PATH)
        find_library(WEBRTC_LIBRARY_DEBUG webrtc PATHS ${VCPKG_INSTALL_ROOT}/debug/lib NO_DEFAULT_PATH)
        select_library_configurations(WEBRTC)
        target_link_libraries(${TARGET_NAME} ${WEBRTC_LIBRARIES})
    endif()


endmacro()
