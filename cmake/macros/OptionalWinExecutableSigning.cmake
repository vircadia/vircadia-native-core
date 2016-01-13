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
  if (WIN32 AND (PRODUCTION_BUILD OR PR_BUILD))
    if (DEFINED ENV{HF_PFX_FILE})
      if (DEFINED ENV{HF_PFX_PASSPHRASE})
        # find signtool
        find_program(SIGNTOOL_EXEC signtool PATHS "C:/Program Files (x86)/Windows Kits/8.1" PATH_SUFFIXES "bin/x64")

        if (NOT SIGNTOOL_EXEC)
          message(FATAL_ERROR "Code signing of executables was requested but signtool.exe could not be found.")
        endif ()

        if (NOT EXECUTABLE_NAME)
          set(EXECUTABLE_NAME $<TARGET_FILE_NAME:${TARGET_NAME}>)
        endif ()

        # setup the post install command to sign the executable

        install(CODE "\
          message(STATUS \"Signing ${TARGET_NAME} with signtool.\")
          execute_process(COMMAND ${SIGNTOOL_EXEC} sign /f $ENV{HF_PFX_FILE}\
            /p $ENV{HF_PFX_PASSPHRASE} /tr http://tsa.starfieldtech.com\
            /td SHA256 \${CMAKE_INSTALL_PREFIX}/${EXECUTABLE_NAME})
          "
          COMPONENT ${EXECUTABLE_COMPONENT}
        )
      else ()
        message(FATAL_ERROR "HF_PFX_PASSPHRASE must be set for executables to be signed.")
      endif ()
    else ()
      message(WARNING "Producing a production build or PR build but not code signing since HF_PFX_FILE is not set.")
    endif ()
  endif ()
endmacro()
