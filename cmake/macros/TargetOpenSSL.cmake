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
        set(OPENSSL_INCLUDE_DIR "${OPENSSL_INSTALL_DIR}/include" CACHE TYPE INTERNAL)
        set(OPENSSL_LIBRARIES "${OPENSSL_INSTALL_DIR}/lib/libcrypto.a;${OPENSSL_INSTALL_DIR}/lib/libssl.a" CACHE TYPE INTERNAL)
    else()

        find_package(OpenSSL REQUIRED)

        if (APPLE AND ${OPENSSL_INCLUDE_DIR} STREQUAL "/usr/include")
            # this is a user on OS X using system OpenSSL, which is going to throw warnings since they're deprecating for their common crypto
            message(WARNING "The found version of OpenSSL is the OS X system version. This will produce deprecation warnings."
            "\nWe recommend you install a newer version (at least 1.0.1h) in a different directory and set OPENSSL_ROOT_DIR in your env so Cmake can find it.")
        endif()

    endif()

    include_directories(SYSTEM "${OPENSSL_INCLUDE_DIR}")
    target_link_libraries(${TARGET_NAME} ${OPENSSL_LIBRARIES})
endmacro()
