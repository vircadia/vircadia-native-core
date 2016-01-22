#
#  ManuallyInstallSSLEay.cmake
#
#  Created by Stephen Birarda on 1/15/16.
#  Copyright 2014 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

macro(manually_install_ssl_eay)

  # on windows we have had issues with targets missing ssleay library
  # not convinced we actually need it (I assume it would show up in the dependency tree)
  # but to be safe we install it manually beside the current target
  if (WIN32)
    # we need to find SSL_EAY_LIBRARY_* so we can install it beside this target
    # so we have to call find_package(OpenSSL) here even though this target may not specifically need it
    find_package(OpenSSL REQUIRED)

    install(
      FILES "${OPENSSL_DLL_PATH}/ssleay32.dll"
      DESTINATION ${TARGET_INSTALL_DIR}
      COMPONENT ${TARGET_INSTALL_COMPONENT}
    )
  endif()

endmacro()
