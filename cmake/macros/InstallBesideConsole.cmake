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
    set(COMPONENT_DESTINATION "bin/Server Console.app/Contents/MacOS/")
  else ()
    set(COMPONENT_DESTINATION bin)
  endif ()

  install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION ${COMPONENT_DESTINATION} COMPONENT ${SERVER_COMPONENT})
endmacro()
