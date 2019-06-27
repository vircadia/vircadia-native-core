#
#  Copyright 2019 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#
macro(TARGET_WEBRTC)

if (ANDROID)
else()
    target_include_directories(${TARGET_NAME} SYSTEM PUBLIC "${VCPKG_INSTALL_ROOT}/include/webrtc")
    find_library(WEBRTC_LIBRARY NAMES webrtc PATHS ${VCPKG_INSTALL_ROOT}/lib/ NO_DEFAULT_PATH)
    target_link_libraries(${TARGET_NAME} ${WEBRTC_LIBRARY})
endif()

endmacro()
