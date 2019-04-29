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
                -DBUNDLE_EXECUTABLE="$<TARGET_FILE:${TARGET_NAME}>"
                -DBUNDLE_PLUGIN_DIR="$<TARGET_FILE_DIR:${TARGET_NAME}>/${PLUGIN_PATH}"
                -P "${CMAKE_CURRENT_BINARY_DIR}/FixupBundlePostBuild.cmake"
        )

        find_program(WINDEPLOYQT_COMMAND windeployqt PATHS ${QT_DIR}/bin NO_DEFAULT_PATH)

        if (NOT WINDEPLOYQT_COMMAND)
            message(FATAL_ERROR "Could not find windeployqt at ${QT_DIR}/bin. windeployqt is required.")
        endif ()

        # add a post-build command to call windeployqt to copy Qt plugins
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND CMD /C "SET PATH=${QT_DIR}/bin;%PATH% && ${WINDEPLOYQT_COMMAND}\
                ${EXTRA_DEPLOY_OPTIONS} $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>,$<CONFIG:RelWithDebInfo>>:--release>\
                --no-compiler-runtime --no-opengl-sw --no-angle -no-system-d3d-compiler \"$<TARGET_FILE:${TARGET_NAME}>\""
        )

        set(QTAUDIO_PATH "$<TARGET_FILE_DIR:${TARGET_NAME}>/audio")
        set(QTAUDIO_WIN7_PATH "$<TARGET_FILE_DIR:${TARGET_NAME}>/audioWin7/audio")
        set(QTAUDIO_WIN8_PATH "$<TARGET_FILE_DIR:${TARGET_NAME}>/audioWin8/audio")

        # copy qtaudio_wasapi.dll and qtaudio_windows.dll in the correct directories for runtime selection
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${QTAUDIO_WIN7_PATH}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${QTAUDIO_WIN8_PATH}"
            # copy release DLLs
            COMMAND if exist "${QTAUDIO_PATH}/qtaudio_windows.dll" ( ${CMAKE_COMMAND} -E copy "${QTAUDIO_PATH}/qtaudio_windows.dll" "${QTAUDIO_WIN7_PATH}" )
            COMMAND if exist "${QTAUDIO_PATH}/qtaudio_windows.dll" ( ${CMAKE_COMMAND} -E copy "${WASAPI_DLL_PATH}/qtaudio_wasapi.dll" "${QTAUDIO_WIN8_PATH}" )
            # copy debug DLLs
            COMMAND if exist "${QTAUDIO_PATH}/qtaudio_windowsd.dll" ( ${CMAKE_COMMAND} -E copy "${QTAUDIO_PATH}/qtaudio_windowsd.dll" "${QTAUDIO_WIN7_PATH}" )
            COMMAND if exist "${QTAUDIO_PATH}/qtaudio_windowsd.dll" ( ${CMAKE_COMMAND} -E copy "${WASAPI_DLL_PATH}/qtaudio_wasapid.dll" "${QTAUDIO_WIN8_PATH}" )
            # remove directory
            COMMAND  ${CMAKE_COMMAND} -E remove_directory "${QTAUDIO_PATH}"
        )
    elseif (APPLE)
        find_program(MACDEPLOYQT_COMMAND macdeployqt PATHS "${QT_DIR}/bin" NO_DEFAULT_PATH)

        if (NOT MACDEPLOYQT_COMMAND)
            message(FATAL_ERROR "Could not find macdeployqt at ${QT_DIR}/bin. macdeployqt is required.")
        endif ()

        # add a post-build command to call macdeployqt to copy Qt plugins
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            (COMMAND ${MACDEPLOYQT_COMMAND}\${CMAKE_INSTALL_PREFIX}/${_INTERFACE_INSTALL_PATH}/ -verbose=2 -qmldir=${CMAKE_SOURCE_DIR}/interface/resources/qml/
        )

        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND CMD /C "SET PATH=${QT_DIR}/bin;%PATH% && ${WINDEPLOYQT_COMMAND}\
                ${EXTRA_DEPLOY_OPTIONS} $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>,$<CONFIG:RelWithDebInfo>>:--release>\
                --no-compiler-runtime --no-opengl-sw --no-angle -no-system-d3d-compiler \"$<TARGET_FILE:${TARGET_NAME}>\""
        )
    endif ()
endmacro()
