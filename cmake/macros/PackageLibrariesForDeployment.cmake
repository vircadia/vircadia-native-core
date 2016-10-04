#
#  PackageLibrariesForDeployment.cmake
#  cmake/macros
#
#  Copyright 2015 High Fidelity, Inc.
#  Created by Stephen Birarda on February 17, 2014
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

macro(PACKAGE_LIBRARIES_FOR_DEPLOYMENT)
  if (WIN32)
    configure_file(
      ${HF_CMAKE_DIR}/templates/FixupBundlePostBuild.cmake.in
      ${CMAKE_CURRENT_BINARY_DIR}/FixupBundlePostBuild.cmake
      @ONLY
    )

    set(PLUGIN_PATH "plugins")

    # add a post-build command to copy DLLs beside the executable
    add_custom_command(
      TARGET ${TARGET_NAME}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND}
        -DBUNDLE_EXECUTABLE=$<TARGET_FILE:${TARGET_NAME}>
        -DBUNDLE_PLUGIN_DIR=$<TARGET_FILE_DIR:${TARGET_NAME}>/${PLUGIN_PATH}
        -P ${CMAKE_CURRENT_BINARY_DIR}/FixupBundlePostBuild.cmake
    )

    find_program(WINDEPLOYQT_COMMAND windeployqt PATHS ${QT_DIR}/bin NO_DEFAULT_PATH)

    if (NOT WINDEPLOYQT_COMMAND)
      message(FATAL_ERROR "Could not find windeployqt at ${QT_DIR}/bin. windeployqt is required.")
    endif ()

    # add a post-build command to call windeployqt to copy Qt plugins
    add_custom_command(
      TARGET ${TARGET_NAME}
      POST_BUILD
      COMMAND CMD /C "SET PATH=%PATH%;${QT_DIR}/bin && ${WINDEPLOYQT_COMMAND} ${EXTRA_DEPLOY_OPTIONS} $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>,$<CONFIG:RelWithDebInfo>>:--release> $<TARGET_FILE:${TARGET_NAME}>"
    )

    set(QTAUDIO_PATH $<TARGET_FILE_DIR:${TARGET_NAME}>/audio)

    if (DEPLOY_PACKAGE)
      # copy qtaudio_wasapi.dll alongside qtaudio_windows.dll, and let the installer resolve
      add_custom_command(
        TARGET ${TARGET_NAME}
        POST_BUILD
        COMMAND if exist ${QTAUDIO_PATH}/qtaudio_windows.dll ( ${CMAKE_COMMAND} -E copy ${WASAPI_DLL_PATH}/qtaudio_wasapi.dll ${QTAUDIO_PATH} && ${CMAKE_COMMAND} -E copy ${WASAPI_DLL_PATH}/qtaudio_wasapi.pdb ${QTAUDIO_PATH} )
        COMMAND if exist ${QTAUDIO_PATH}/qtaudio_windowsd.dll ( ${CMAKE_COMMAND} -E copy ${WASAPI_DLL_PATH}/qtaudio_wasapid.dll ${QTAUDIO_PATH} && ${CMAKE_COMMAND} -E copy ${WASAPI_DLL_PATH}/qtaudio_wasapid.pdb ${QTAUDIO_PATH} )
      )
    elseif (${CMAKE_SYSTEM_VERSION} VERSION_LESS 6.2)
      # continue using qtaudio_windows.dll on Windows 7
    else ()
      # replace qtaudio_windows.dll with qtaudio_wasapi.dll on Windows 8/8.1/10
      add_custom_command(
        TARGET ${TARGET_NAME}
        POST_BUILD
        COMMAND if exist ${QTAUDIO_PATH}/qtaudio_windows.dll ( ${CMAKE_COMMAND} -E remove ${QTAUDIO_PATH}/qtaudio_windows.dll && ${CMAKE_COMMAND} -E copy ${WASAPI_DLL_PATH}/qtaudio_wasapi.dll ${QTAUDIO_PATH} && ${CMAKE_COMMAND} -E copy ${WASAPI_DLL_PATH}/qtaudio_wasapi.pdb ${QTAUDIO_PATH} )
        COMMAND if exist ${QTAUDIO_PATH}/qtaudio_windowsd.dll ( ${CMAKE_COMMAND} -E remove ${QTAUDIO_PATH}/qtaudio_windowsd.dll && ${CMAKE_COMMAND} -E copy ${WASAPI_DLL_PATH}/qtaudio_wasapid.dll ${QTAUDIO_PATH} && ${CMAKE_COMMAND} -E copy ${WASAPI_DLL_PATH}/qtaudio_wasapid.pdb ${QTAUDIO_PATH} )
      )
    endif ()   

  endif ()
endmacro()
