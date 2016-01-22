#
#  ManuallyInstallMSVC.cmake
#  cmake/macros
#
#  Copyright 2016 High Fidelity, Inc.
#  Created by Stephen Birarda on January 22nd, 2016
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

# On windows fixup_bundle does not find msvcr120.dll and msvcp120.dll that our targets need to be re-distributable.
# We use this macro to find them and manually install them beside the targets

macro(manually_install_msvc)
  if (WIN32)
    # look for the msvcr DLLs required by this target
    find_path(MSVC_DLL_PATH msvcr120.dll PATHS "C:/Windows/System32" NO_DEFAULT_PATH)

    if (MSVC_DLL_PATH-NOTFOUND)
      # we didn't get the path to the DLLs - on production or PR build this is a fail
      if (PRODUCTION_BUILD OR PR_BUILD)
        message(FATAL_ERROR "Did not find MSVC_DLL_PATH for msvcr120.dll and msvcp120.dll.\
          Both are required to package re-distributable installer."
        )
      endif ()
    else ()
      # manually install the two DLLs for this component
      install(
        FILES "${MSVC_DLL_PATH}/msvcr120.dll" "${MSVC_DLL_PATH}/msvcp120.dll"
        DESTINATION ${TARGET_INSTALL_DIR}
        COMPONENT ${TARGET_INSTALL_COMPONENT}
      )
    endif ()

   endif ()
endmacro()
