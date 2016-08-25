#
#  GenerateInstallers.cmake
#  cmake/macros
#
#  Created by Leonardo Murillo on 12/16/2015.
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

macro(GENERATE_INSTALLERS)
  include(CPackComponent)

  set(CPACK_MODULE_PATH ${CPACK_MODULE_PATH} "${HF_CMAKE_DIR}/templates")

  set(_DISPLAY_NAME ${BUILD_ORGANIZATION})

  set(CPACK_PACKAGE_NAME ${_DISPLAY_NAME})
  set(CPACK_PACKAGE_VENDOR "High Fidelity")
  set(CPACK_PACKAGE_VERSION ${BUILD_VERSION})
  set(CPACK_PACKAGE_FILE_NAME "HighFidelity-Beta-${BUILD_VERSION}")
  set(CPACK_NSIS_DISPLAY_NAME ${_DISPLAY_NAME})
  set(CPACK_NSIS_PACKAGE_NAME ${_DISPLAY_NAME})
  set(CPACK_PACKAGE_INSTALL_DIRECTORY ${_DISPLAY_NAME})

  if (WIN32)
    # include CMake module that will install compiler system libraries
    # so that we have msvcr120 and msvcp120 installed with targets
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ${INTERFACE_INSTALL_DIR})

    # as long as we're including sixense plugin with installer
    # we need re-distributables for VS 2011 as well
    # this should be removed if/when sixense support is pulled
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS
      "${EXTERNALS_BINARY_DIR}/sixense/project/src/sixense/samples/win64/msvcr100.dll"
      "${EXTERNALS_BINARY_DIR}/sixense/project/src/sixense/samples/win64/msvcp100.dll"
    )

    include(InstallRequiredSystemLibraries)

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
  elseif (APPLE)
    # produce a drag and drop DMG on OS X
    set(CPACK_GENERATOR "DragNDrop")

    set(CPACK_PACKAGE_INSTALL_DIRECTORY "/")
    set(CPACK_PACKAGING_INSTALL_PREFIX /)
    set(CPACK_OSX_PACKAGE_VERSION ${CMAKE_OSX_DEPLOYMENT_TARGET})

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

  # configure a cpack properties file for custom variables in template
  set(CPACK_CONFIGURED_PROP_FILE "${CMAKE_CURRENT_BINARY_DIR}/CPackCustomProperties.cmake")
  configure_file("${HF_CMAKE_DIR}/templates/CPackProperties.cmake.in" ${CPACK_CONFIGURED_PROP_FILE})
  set(CPACK_PROPERTIES_FILE ${CPACK_CONFIGURED_PROP_FILE})

  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

  cpack_add_component(${CLIENT_COMPONENT} DISPLAY_NAME "High Fidelity Interface")
  cpack_add_component(${SERVER_COMPONENT} DISPLAY_NAME "High Fidelity Sandbox")

  include(CPack)
endmacro()
