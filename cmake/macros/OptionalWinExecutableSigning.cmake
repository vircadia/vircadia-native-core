#
#  OptionalWinExecutableSigning.cmake
#  cmake/macros
#
#  Copyright 2016 High Fidelity, Inc.
#  Created by Stephen Birarda on January 12, 2016
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

macro(optional_win_executable_signing)
  if (WIN32 AND PRODUCTION_BUILD AND NOT BYPASS_SIGNING)
    if (DEFINED ENV{HF_PFX_FILE})
      if (DEFINED ENV{HF_PFX_PASSPHRASE})
        message(STATUS "Executable for ${TARGET_NAME} will be signed with SignTool.")

        if (NOT EXECUTABLE_PATH)
          set(EXECUTABLE_PATH "$<TARGET_FILE:${TARGET_NAME}>")
        endif ()

        # setup a post build command to sign the executable
        add_custom_command(
          TARGET ${TARGET_NAME} POST_BUILD
          COMMAND ${SIGNTOOL_EXECUTABLE} sign /fd sha256 /f %HF_PFX_FILE% /p %HF_PFX_PASSPHRASE% /tr ${TIMESERVER_URL} /td SHA256 ${EXECUTABLE_PATH}
        )
      else ()
        message(FATAL_ERROR "HF_PFX_PASSPHRASE must be set for executables to be signed.")
      endif ()
    else ()
      message(WARNING "Creating a production build but not code signing since HF_PFX_FILE is not set.")
    endif ()
  endif ()
endmacro()
