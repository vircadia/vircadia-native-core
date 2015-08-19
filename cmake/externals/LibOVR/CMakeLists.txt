include(ExternalProject)
include(SelectLibraryConfigurations)

set(EXTERNAL_NAME LibOVR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (WIN32)

  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL http://static.oculus.com/sdk-downloads/0.6.0.1/Public/1435190862/ovr_sdk_win_0.6.0.1.zip
    URL_MD5 4b3ef825f9a1d6d3035c9f6820687da9
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
  )

  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

  # FIXME need to account for different architectures 
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/LibOVR/Include CACHE TYPE INTERNAL)
  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/LibOVR/Lib/Windows/x64/Release/VS2013/LibOVR.lib CACHE TYPE INTERNAL)
  else()
    set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/LibOVR/Lib/Windows/Win32/Release/VS2013/LibOVR.lib CACHE TYPE INTERNAL)
  endif()

elseif(APPLE)

  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL http://static.oculus.com/sdk-downloads/ovr_sdk_macos_0.5.0.1.tar.gz 
    URL_MD5 0a0785a04fb285f64f62267388344ad6
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
  )

  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)
  # In theory we should use the Headers path inside the framework, as seen here
  # but unfortunately Oculus doesn't seem to have figured out automated testing
  # so they released a framework with missing headers.
  #set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/LibOVR/Lib/Mac/Release/LibOVR.framework/Headers/ CACHE TYPE INTERNAL)

  # Work around the broken framework by using a different path for the headers. 
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/LibOVR/Include CACHE TYPE INTERNAL)
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/LibOVR/Lib/Mac/Release/LibOVR.framework/LibOVR CACHE TYPE INTERNAL)



elseif(NOT ANDROID)

  # http://static.oculus.com/sdk-downloads/ovr_sdk_linux_0.4.4.tar.xz
  # ec3bd8cff4a1461b4e21210e7feb0572
  ExternalProject_Add(
    ${EXTERNAL_NAME}
    PREFIX ${EXTERNAL_NAME}
    GIT_REPOSITORY https://github.com/jherico/OculusSDK.git
    GIT_TAG 0d6f0cf110ea7566fc6d64b8d4fe6bb881d9cff5
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    LOG_DOWNLOAD ON
  )
  
  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)
  ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)
    
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/libovr.a CACHE TYPE INTERNAL)
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG "" CACHE TYPE INTERNAL)
    
  find_package(Threads REQUIRED)
  find_package(X11 REQUIRED)

  # Check for XRandR (modern resolution switching and gamma control)
  if (NOT X11_Xrandr_FOUND)
      message(FATAL_ERROR "The RandR library and headers were not found")
  endif()

  set(${EXTERNAL_NAME_UPPER}_LIBRARY_EXTRAS rt udev ${CMAKE_THREAD_LIBS_INIT} ${X11_X11_LIB} ${X11_Xrandr_LIB})

  select_library_configurations(${EXTERNAL_NAME_UPPER})
  
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/LibOVR/Include ${SOURCE_DIR}/LibOVR/Src  CACHE TYPE INTERNAL)
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${${EXTERNAL_NAME_UPPER}_LIBRARY} ${${EXTERNAL_NAME_UPPER}_LIBRARY_EXTRAS} CACHE TYPE INTERNAL)
endif()

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")


