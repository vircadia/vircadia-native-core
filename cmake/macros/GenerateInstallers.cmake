#
#  GenerateInstallers.cmake
#  cmake/macros
#
#  Created by Leonardo Murillo on 12/16/2015.
#  Copyright 2015 High Fidelity, Inc.
#  Copyright 2021 Vircadia contributors.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

macro(GENERATE_INSTALLERS)
  include(CPackComponent)

  set(CPACK_MODULE_PATH ${CPACK_MODULE_PATH} "${HF_CMAKE_DIR}/templates")


  if (CLIENT_ONLY)
    set(_PACKAGE_NAME_EXTRA "-Interface")
    set(INSTALLER_TYPE "client_only")
    string(REGEX REPLACE "Vircadia" "Vircadia Interface" _DISPLAY_NAME ${BUILD_ORGANIZATION})
  elseif (SERVER_ONLY)
    set(_PACKAGE_NAME_EXTRA "-Server")
    set(INSTALLER_TYPE "server_only")
    string(REGEX REPLACE "Vircadia" "Vircadia Server" _DISPLAY_NAME ${BUILD_ORGANIZATION})
  else ()
    set(_DISPLAY_NAME ${BUILD_ORGANIZATION})
    set(INSTALLER_TYPE "full")
  endif ()

  set(CPACK_PACKAGE_NAME ${_DISPLAY_NAME})
  set(CPACK_PACKAGE_VENDOR "Vircadia")
  set(CPACK_PACKAGE_VERSION ${BUILD_VERSION})
  set(CPACK_PACKAGE_FILE_NAME "Vircadia${_PACKAGE_NAME_EXTRA}-${BUILD_VERSION}-${RELEASE_NAME}")
  set(CPACK_NSIS_DISPLAY_NAME ${_DISPLAY_NAME})
  set(CPACK_NSIS_PACKAGE_NAME ${_DISPLAY_NAME})
  if (PR_BUILD)
    set(CPACK_NSIS_COMPRESSOR "bzip2")
  endif ()
  set(CPACK_PACKAGE_INSTALL_DIRECTORY ${_DISPLAY_NAME})


  if (WIN32)
    set(CPACK_NSIS_MUI_ICON "${HF_CMAKE_DIR}/installer/installer.ico")

    # install and reference the Add/Remove icon
    set(ADD_REMOVE_ICON_NAME "installer.ico")
    set(ADD_REMOVE_ICON_BAD_PATH "${HF_CMAKE_DIR}/installer/${ADD_REMOVE_ICON_NAME}")
    fix_path_for_nsis(${ADD_REMOVE_ICON_BAD_PATH} ADD_REMOVE_ICON_PATH)
    set(CPACK_NSIS_INSTALLED_ICON_NAME ${ADD_REMOVE_ICON_NAME})

    # use macro to put backslashes in header image path since nsis requires them
    set(_INSTALLER_HEADER_BAD_PATH "${HF_CMAKE_DIR}/installer/installer-header.bmp")
    set(INSTALLER_HEADER_IMAGE "")
    fix_path_for_nsis(${_INSTALLER_HEADER_BAD_PATH} INSTALLER_HEADER_IMAGE)

    set(_UNINSTALLER_HEADER_BAD_PATH "${HF_CMAKE_DIR}/installer/uninstaller-header.bmp")
    set(UNINSTALLER_HEADER_IMAGE "")
    fix_path_for_nsis(${_UNINSTALLER_HEADER_BAD_PATH} UNINSTALLER_HEADER_IMAGE)

    # we use external libraries that still need the 120 (VS2013) redistributables
    # so we include them as well until those external libraries are updated
    # to use the redistributables that match what we build our applications for
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS
      "C:/Windows/System32/msvcp120.dll"
      "C:/Windows/System32/msvcr120.dll"
    )

    set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ${INTERFACE_INSTALL_DIR})
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT ${CLIENT_COMPONENT})
    include(InstallRequiredSystemLibraries)

    if (CLIENT_ONLY OR SERVER_ONLY)
      set(CPACK_MONOLITHIC_INSTALL 1)
    endif ()

    # setup conditional checks for server component selection depending on
    # the inclusion of the server component at all
    if (CLIENT_ONLY)
      set(SERVER_COMPONENT_CONDITIONAL "0 == 1")
      set(CLIENT_COMPONENT_CONDITIONAL "1 == 1")
    elseif (SERVER_ONLY)
      set(SERVER_COMPONENT_CONDITIONAL "1 == 1")
      set(CLIENT_COMPONENT_CONDITIONAL "0 == 1")
    else ()
      set(SERVER_COMPONENT_CONDITIONAL "\\\${SectionIsSelected} \\\${${SERVER_COMPONENT}}")
      set(CLIENT_COMPONENT_CONDITIONAL "\\\${SectionIsSelected} \\\${${CLIENT_COMPONENT}}")
    endif ()
  elseif (APPLE)
    # produce a drag and drop DMG on OS X
    set(CPACK_GENERATOR "DragNDrop")

    set(CPACK_PACKAGE_INSTALL_DIRECTORY "/")
    set(CPACK_PACKAGING_INSTALL_PREFIX /)
    set(CPACK_OSX_PACKAGE_VERSION ${CMAKE_OSX_DEPLOYMENT_TARGET})

    # Create folder if used.
    if (NOT INTERFACE_INSTALL_DIR STREQUAL ".")
      # make sure a High Fidelity directory exists, in case this hits prior to other installs
      install(CODE "file(MAKE_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/${DMG_SUBFOLDER_NAME}\")")

      # add the resource file to the Icon file inside the folder
      install(CODE
        "execute_process(COMMAND Rez -append ${DMG_SUBFOLDER_ICON} -o \${CMAKE_INSTALL_PREFIX}/${ESCAPED_DMG_SUBFOLDER_NAME}/Icon\\r)"
      )

      # modify the folder to use that custom icon
      install(CODE "execute_process(COMMAND SetFile -a C \${CMAKE_INSTALL_PREFIX}/${ESCAPED_DMG_SUBFOLDER_NAME})")

      # hide the special Icon? file
      install(CODE "execute_process(COMMAND SetFile -a V \${CMAKE_INSTALL_PREFIX}/${ESCAPED_DMG_SUBFOLDER_NAME}/Icon\\r)")
    endif ()

  endif ()

  # configure a cpack properties file for custom variables in template
  set(CPACK_CONFIGURED_PROP_FILE "${CMAKE_CURRENT_BINARY_DIR}/CPackCustomProperties.cmake")
  configure_file("${HF_CMAKE_DIR}/templates/CPackProperties.cmake.in" ${CPACK_CONFIGURED_PROP_FILE})
  set(CPACK_PROPERTIES_FILE ${CPACK_CONFIGURED_PROP_FILE})

  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

  if (BUILD_CLIENT)
    cpack_add_component(${CLIENT_COMPONENT} DISPLAY_NAME "Vircadia Interface")
  endif ()

  if (BUILD_SERVER)
    cpack_add_component(${SERVER_COMPONENT} DISPLAY_NAME "Vircadia Server")
  endif ()

  include(CPack)
endmacro()
