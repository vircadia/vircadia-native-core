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
    set(SHELL_APP_CONTENTS "Components.app/Contents")
    set(COMPONENT_DESTINATION "${SHELL_APP_CONTENTS}/MacOS")

  else ()
    set(COMPONENT_DESTINATION .)
  endif ()

  install(
    TARGETS ${TARGET_NAME}
    RUNTIME DESTINATION ${COMPONENT_DESTINATION}
    COMPONENT ${SERVER_COMPONENT}
  )

  if (APPLE)
    # during the install phase, call fixup to drop the shared libraries for these components in the right place
    set(EXECUTABLE_NEEDING_FIXUP "\${CMAKE_INSTALL_PREFIX}/${COMPONENT_DESTINATION}/${TARGET_NAME}")
    install(CODE "
      include(BundleUtilities)
      fixup_bundle(\"${EXECUTABLE_NEEDING_FIXUP}\" \"\" \"${FIXUP_LIBS}\")
    " COMPONENT ${SERVER_COMPONENT})

    # once installed copy the contents of the shell Components.app to the console application
    set(CONSOLE_INSTALL_PATH "Applications/High Fidelity/Server Console.app")
    set(INSTALLED_CONSOLE_CONTENTS "\${CMAKE_INSTALL_PREFIX}/${CONSOLE_INSTALL_PATH}/Contents")

    set(INSTALLED_SHELL_CONTENTS "\${CMAKE_INSTALL_PREFIX}/${SHELL_APP_CONTENTS}")
    install(CODE "
      file(GLOB FRAMEWORKS \"${INSTALLED_SHELL_CONTENTS}/Frameworks/*\")
      message(STATUS \"Copying \${FRAMEWORKS} to ${INSTALLED_CONSOLE_CONTENTS}/Frameworks\")
      file(COPY \${FRAMEWORKS} DESTINATION \"${INSTALLED_CONSOLE_CONTENTS}/Frameworks\")
      file(GLOB BINARIES \"${INSTALLED_SHELL_CONTENTS}/MacOS/*\")
      message(STATUS \"Copying \${BINARIES} to ${INSTALLED_CONSOLE_CONTENTS}/Frameworks\")
      file(COPY \${BINARIES} DESTINATION \"${INSTALLED_CONSOLE_CONTENTS}/MacOS\")
    " COMPONENT ${SERVER_COMPONENT})

  endif ()

endmacro()
