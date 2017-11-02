#
#  PackageCrashpadForDeployment.cmake
#  cmake/macros
#
#  Copyright 2018 High Fidelity, Inc.
#  Created by Clement Brisset on  01/19/18
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

macro(PACKAGE_CRASHPAD_FOR_DEPLOYMENT)
    get_property(HAS_CRASHPAD GLOBAL PROPERTY HAS_CRASHPAD)

    if (HAS_CRASHPAD)
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${CRASHPAD_HANDLER_EXE_PATH} "$<TARGET_FILE_DIR:${TARGET_NAME}>/"
        )
        install(
            PROGRAMS ${CRASHPAD_HANDLER_EXE_PATH}
            DESTINATION ${CLIENT_COMPONENT}
            COMPONENT ${INTERFACE_INSTALL_DIR}
        )
    endif ()
endmacro()
