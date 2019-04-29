#
#  FixupNitpick.cmake
#  cmake/macros
#
#  Copyright 2019 High Fidelity, Inc.
#  Created by Nissim Hadar on January 14th, 2016
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

macro(fixup_nitpick)
    if (APPLE)
        string(REPLACE " " "\\ " ESCAPED_BUNDLE_NAME ${NITPICK_BUNDLE_NAME})
        string(REPLACE " " "\\ " ESCAPED_INSTALL_PATH ${NITPICK_INSTALL_DIR})
        set(_NITPICK_INSTALL_PATH "${ESCAPED_INSTALL_PATH}/${ESCAPED_BUNDLE_NAME}.app")

        find_program(MACDEPLOYQT_COMMAND macdeployqt PATHS "${QT_DIR}/bin" NO_DEFAULT_PATH)

        if (NOT MACDEPLOYQT_COMMAND)
            message(FATAL_ERROR "Could not find macdeployqt at ${QT_DIR}/bin.\
                It is required to produce a relocatable nitpick application.\
                Check that the variable QT_DIR points to your Qt installation.\
            ")
        endif ()

        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${MACDEPLOYQT_COMMAND}\${CMAKE_INSTALL_PREFIX}/${_INTERFACE_INSTALL_PATH}/ -verbose=2 -qmldir=${CMAKE_SOURCE_DIR}/interface/resources/qml/
        )
    endif ()
endmacro()
