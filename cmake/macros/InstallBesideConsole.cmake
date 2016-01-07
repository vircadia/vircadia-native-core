#
#  InstallBesideConsole.cmake
#  cmake/macros
#
#  Copyright 2016 High Fidelity, Inc.
#  Created by Stephen Birarda on January 5th, 2016
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

macro(install_beside_console)
  # install this component beside the installed server-console executable
  if (APPLE)
    set(CONSOLE_APP_CONTENTS "${CONSOLE_INSTALL_APP_PATH}/Contents")
    set(COMPONENT_DESTINATION "${CONSOLE_APP_CONTENTS}/MacOS/Components.app/Contents/MacOS")
  else ()
    set(COMPONENT_DESTINATION ${CONSOLE_INSTALL_DIR})
  endif ()

  install(
    TARGETS ${TARGET_NAME}
    RUNTIME DESTINATION ${COMPONENT_DESTINATION}
    COMPONENT ${SERVER_COMPONENT}
  )

  if (TARGET_NAME STREQUAL domain-server)
    if (APPLE)
      set(RESOURCES_DESTINATION ${COMPONENT_DESTINATION})
    else ()
      set(RESOURCES_DESTINATION ${CONSOLE_INSTALL_DIR})
    endif ()

    # install the resources folder for the domain-server where its executable will be
    install(
      DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources
      DESTINATION ${RESOURCES_DESTINATION}
      USE_SOURCE_PERMISSIONS
      COMPONENT ${SERVER_COMPONENT}
    )
  endif ()

  if (APPLE)
    # during the install phase, call fixup to drop the shared libraries for these components in the right place
    set(EXECUTABLE_NEEDING_FIXUP "\${CMAKE_INSTALL_PREFIX}/${COMPONENT_DESTINATION}/${TARGET_NAME}")
    install(CODE "
      include(BundleUtilities)
      fixup_bundle(\"${EXECUTABLE_NEEDING_FIXUP}\" \"\" \"${FIXUP_LIBS}\")
    " COMPONENT ${SERVER_COMPONENT})
  endif ()

endmacro()
