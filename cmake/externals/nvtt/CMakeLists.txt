include(ExternalProject)
include(SelectLibraryConfigurations)

set(EXTERNAL_NAME nvtt)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (NOT ANDROID)
  if (WIN32)
    ExternalProject_Add(
      ${EXTERNAL_NAME}
      URL http://s3.amazonaws.com/hifi-public/dependencies/nvtt-win-2.1.0.zip
      URL_MD5 3ea6eeadbcc69071acf9c49ba565760e
      LOG_DOWNLOAD 1
    )

    ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

    set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE PATH "Location of NVTT include directory")

    set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${SOURCE_DIR}/Release/x64/nvtt.lib CACHE FILEPATH "Path to NVTT static release library")
    set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${SOURCE_DIR}/Debug/x64/nvtt.lib CACHE FILEPATH "Path to NVTT static debug library")

    set(${EXTERNAL_NAME_UPPER}_DLL_PATH "${SOURCE_DIR}/$<$<CONFIG:Debug>:Debug>$<$<NOT:$<CONFIG:Debug>>:Release>/x64" CACHE PATH "Location of NVTT DLL")
  else ()
    ExternalProject_Add(
      ${EXTERNAL_NAME}
      URL http://hifi-public.s3.amazonaws.com/dependencies/nvidia-texture-tools-2.1.0.zip
      URL_MD5 79842ea185c8c95aaa70cee91e77f032
      CONFIGURE_COMMAND CMAKE_ARGS  ${ANDROID_CMAKE_ARGS} -DNVTT_SHARED=1 -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
      LOG_DOWNLOAD 1
      LOG_CONFIGURE 1
      LOG_BUILD 1
    )

    ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

    set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR}/include CACHE PATH "Location of NVTT include directory")

    if (APPLE)
      set(_LIB_EXT "dylib")
    else ()
      set(_LIB_EXT "so")
    endif ()

    set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/libnvtt.${_LIB_EXT} CACHE FILEPATH "Path to NVTT library")

    if (APPLE)
      # on OS X we have to use install_name_tool to fix the paths found in the NVTT shared libraries
      # so that they can be found and linked during the linking phase
      set(_NVTT_LIB_DIR "${INSTALL_DIR}/lib")

      # first fix the install names of all present libraries
      ExternalProject_Add_Step(
        ${EXTERNAL_NAME}
        change-install-name
        COMMENT "Calling install_name_tool on NVTT libraries to fix install name for dylib linking"
        COMMAND ${CMAKE_COMMAND} -DINSTALL_NAME_LIBRARY_DIR=${_NVTT_LIB_DIR} -P ${EXTERNAL_PROJECT_DIR}/OSXInstallNameChange.cmake
        DEPENDEES install
        WORKING_DIRECTORY <INSTALL_DIR>
        LOG 1
      )

      # then, for the main library (libnvtt) fix the paths to the dependency libraries (core, image, math)
      ExternalProject_Add_Step(
        ${EXTERNAL_NAME}
        change-dependency-paths
        COMMENT "Calling install_name_tool on NVTT libraries to fix paths for dependency libraries"
        COMMAND install_name_tool -change libnvimage.dylib ${INSTALL_DIR}/lib/libnvimage.dylib libnvtt.dylib
        COMMAND install_name_tool -change libnvcore.dylib  ${INSTALL_DIR}/lib/libnvcore.dylib libnvtt.dylib
        COMMAND install_name_tool -change libnvmath.dylib  ${INSTALL_DIR}/lib/libnvmath.dylib libnvtt.dylib
        COMMAND install_name_tool -change libnvcore.dylib  ${INSTALL_DIR}/lib/libnvcore.dylib libnvimage.dylib
        COMMAND install_name_tool -change libnvmath.dylib  ${INSTALL_DIR}/lib/libnvmath.dylib libnvimage.dylib
        COMMAND install_name_tool -change libnvcore.dylib  ${INSTALL_DIR}/lib/libnvcore.dylib libnvmath.dylib
        DEPENDEES install
        WORKING_DIRECTORY <INSTALL_DIR>/lib
        LOG 1
      )
    endif ()
  endif ()

  # Hide this external target (for IDE users)
  set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
endif()
