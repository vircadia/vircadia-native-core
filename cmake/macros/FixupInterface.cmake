#
#  FixupInterface.cmake
#  cmake/macros
#
#  Copyright 2016 High Fidelity, Inc.
#  Created by Stephen Birarda on January 6th, 2016
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

macro(fixup_interface)
  if (APPLE)

    string(REPLACE " " "\\ " ESCAPED_BUNDLE_NAME ${INTERFACE_BUNDLE_NAME})
    string(REPLACE " " "\\ " ESCAPED_INSTALL_PATH ${INTERFACE_INSTALL_DIR})
    set(_INTERFACE_INSTALL_PATH "${ESCAPED_INSTALL_PATH}/${ESCAPED_BUNDLE_NAME}.app")

    find_program(MACDEPLOYQT_COMMAND macdeployqt PATHS "${QT_DIR}/bin" NO_DEFAULT_PATH)

    if (NOT MACDEPLOYQT_COMMAND AND (PRODUCTION_BUILD OR PR_BUILD))
      message(FATAL_ERROR "Could not find macdeployqt at ${QT_DIR}/bin.\
        It is required to produce an relocatable interface application.\
        Check that the environment variable QT_DIR points to your Qt installation.\
      ")
    endif ()

    install(CODE "
      execute_process(COMMAND ${MACDEPLOYQT_COMMAND}\
        \${CMAKE_INSTALL_PREFIX}/${_INTERFACE_INSTALL_PATH}/\
        -verbose=2 -qmldir=${CMAKE_SOURCE_DIR}/interface/resources/qml/\
      )"
      COMPONENT ${CLIENT_COMPONENT}
    )

  endif ()
endmacro()
