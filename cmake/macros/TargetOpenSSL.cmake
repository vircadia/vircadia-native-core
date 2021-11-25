#
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#
macro(TARGET_OPENSSL)
    if (ANDROID)
        set(OPENSSL_INSTALL_DIR ${HIFI_ANDROID_PRECOMPILED}/openssl)
        set(OPENSSL_INCLUDE_DIR "${OPENSSL_INSTALL_DIR}/include" CACHE STRING INTERNAL)
        set(OPENSSL_LIBRARIES "${OPENSSL_INSTALL_DIR}/lib/libcrypto.a;${OPENSSL_INSTALL_DIR}/lib/libssl.a" CACHE STRING INTERNAL)
    else()
    	# using VCPKG for OpenSSL
        find_package(OpenSSL 1.1.0 REQUIRED)
    endif()

    include_directories(SYSTEM "${OPENSSL_INCLUDE_DIR}")
    target_link_libraries(${TARGET_NAME} ${OPENSSL_LIBRARIES})
endmacro()
