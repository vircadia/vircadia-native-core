#
#  InstallBesideInterface.cmake
#  cmake/macros
#
#  Created by Stephen Birarda on January 5th, 2016
#  Copyright 2016 High Fidelity, Inc.
#  Copyright 2021 Vircadia contributors.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

macro(install_beside_interface)
  if (WIN32 OR APPLE)

    message(STATUS, "####### InstallBesideInterface.cmake");

    # install this component beside the installed interface executable
    if (APPLE)
      message(STATUS, "####... TARGET_NAME: ${TARGET_NAME}");
      message(STATUS, "####... UTILS_COMPONENT_INSTALL_DIR: ${UTILS_COMPONENT_INSTALL_DIR}");
      message(STATUS, "####... UTILS_PLUGIN_INSTALL_DIR: ${UTILS_PLUGIN_INSTALL_DIR}");
      message(STATUS, "####... CLIENT_COMPONENT: ${CLIENT_COMPONENT}");

      install(
        TARGETS ${TARGET_NAME}
        RUNTIME DESTINATION ${UTILS_COMPONENT_INSTALL_DIR}
        LIBRARY DESTINATION ${UTILS_PLUGIN_INSTALL_DIR}
        COMPONENT ${CLIENT_COMPONENT}
      )
    else ()
      # setup install of executable and things copied by fixup/windeployqt

      message(STATUS, "####... TARGET_NAME: ${TARGET_NAME}");
      message(STATUS, "####... $<...>/: $<TARGET_FILE_DIR:${TARGET_NAME}>/");
      message(STATUS, "####... UTILS_COMPONENT_INSTALL_DIR: ${UTILS_COMPONENT_INSTALL_DIR}");
      message(STATUS, "####... CLIENT_COMPONENT: ${CLIENT_COMPONENT}");

      install(
        DIRECTORY "$<TARGET_FILE_DIR:${TARGET_NAME}>/"
        DESTINATION ${UTILS_COMPONENT_INSTALL_DIR}
        COMPONENT ${CLIENT_COMPONENT}
        PATTERN "*.pdb" EXCLUDE
        PATTERN "*.lib" EXCLUDE
        PATTERN "*.exp" EXCLUDE
      )

      # on windows for PR and production builds, sign the executable
      set(EXECUTABLE_COMPONENT ${CLIENT_COMPONENT})
      optional_win_executable_signing()
    endif ()

    if (APPLE)
      find_program(MACDEPLOYQT_COMMAND macdeployqt PATHS "${QT_DIR}/bin" NO_DEFAULT_PATH)

      if (NOT MACDEPLOYQT_COMMAND AND (PRODUCTION_BUILD OR PR_BUILD))
        message(FATAL_ERROR "Could not find macdeployqt at ${QT_DIR}/bin.\
          It is required to produce a relocatable interface application.\
          Check that the environment variable QT_DIR points to your Qt installation.\
        ")
      endif ()

      # during the install phase, call macdeployqt to drop the shared libraries for these components in the right place
      set(UTILS_COMPONENTS_BUNDLE_PATH "\${CMAKE_INSTALL_PREFIX}/${UTILS_COMPONENT_APP_PATH}")
      string(REPLACE " " "\\ " ESCAPED_BUNDLE_NAME ${UTILS_COMPONENTS_BUNDLE_PATH})

      set(EXECUTABLE_NEEDING_FIXUP "\${CMAKE_INSTALL_PREFIX}/${UTILS_COMPONENT_INSTALL_DIR}/${TARGET_NAME}")
      string(REPLACE " " "\\ " ESCAPED_EXECUTABLE_NAME ${EXECUTABLE_NEEDING_FIXUP})

      # configure Info.plist for COMPONENT_APP
      install(CODE "
        set(MACOSX_BUNDLE_EXECUTABLE_NAME utils)
        set(MACOSX_BUNDLE_GUI_IDENTIFIER com.highfidelity.utilities)
        set(MACOSX_BUNDLE_BUNDLE_NAME interface\\ Utilities)
        configure_file(${HF_CMAKE_DIR}/templates/MacOSXBundleComponentsInfo.plist.in ${ESCAPED_BUNDLE_NAME}/Contents/Info.plist)
        execute_process(COMMAND ${MACDEPLOYQT_COMMAND} ${ESCAPED_BUNDLE_NAME} -verbose=2 -executable=${ESCAPED_EXECUTABLE_NAME})"
        COMPONENT ${CLIENT_COMPONENT}
      )
    endif()

    # set variables used by manual ssleay library copy
    set(TARGET_INSTALL_DIR ${UTILS_COMPONENT_INSTALL_DIR})
    set(TARGET_INSTALL_COMPONENT ${CLIENT_COMPONENT})
    manually_install_openssl_for_qt()

  endif ()
endmacro()
