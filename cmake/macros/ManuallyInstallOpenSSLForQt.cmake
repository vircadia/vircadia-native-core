#
#  ManuallyInstallOpenSSLforQt.cmake
#
#  Created by Stephen Birarda on 1/15/16.
#  Copyright 2014 High Fidelity, Inc.
#  Copyright 2020 Vircadia contributors.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

macro(manually_install_openssl_for_qt)

  # Qt dynamically links OpenSSL if it can find it on the user's machine
  # We want to avoid it being found somewhere random and have it not being a compatible version
  # So even though we don't need the dynamic version of OpenSSL for our direct-use purposes
  # we use this macro to include the two SSL DLLs with the targets using QtNetwork
  if (WIN32)
    # we have to call find_package(OpenSSL) here even though this target may not directly need it
    find_package(OpenSSL REQUIRED)

    install(
      FILES "${VCPKG_INSTALL_ROOT}/bin/libcrypto-1_1-x64.dll"
      DESTINATION ${TARGET_INSTALL_DIR}
      COMPONENT ${TARGET_INSTALL_COMPONENT}
    )

    install(
      FILES "${VCPKG_INSTALL_ROOT}/bin/libssl-1_1-x64.dll"
      DESTINATION ${TARGET_INSTALL_DIR}
      COMPONENT ${TARGET_INSTALL_COMPONENT}
    )
  endif()

endmacro()
