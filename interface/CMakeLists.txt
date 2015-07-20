set(TARGET_NAME interface)
project(${TARGET_NAME})

# set a default root dir for each of our optional externals if it was not passed
set(OPTIONAL_EXTERNALS "Faceshift" "Sixense" "LeapMotion" "RtMidi" "SDL2" "RSSDK")
foreach(EXTERNAL ${OPTIONAL_EXTERNALS})
  string(TOUPPER ${EXTERNAL} ${EXTERNAL}_UPPERCASE)
  if (NOT ${${EXTERNAL}_UPPERCASE}_ROOT_DIR)
    string(TOLOWER ${EXTERNAL} ${EXTERNAL}_LOWERCASE)
    set(${${EXTERNAL}_UPPERCASE}_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/${${EXTERNAL}_LOWERCASE}")
  endif ()
endforeach()

find_package(Qt5LinguistTools REQUIRED)
find_package(Qt5LinguistToolsMacros)

if (DEFINED ENV{JOB_ID})
  set(BUILD_SEQ $ENV{JOB_ID})
elseif (DEFINED ENV{ghprbPullId})
  set(BUILD_SEQ "PR: $ENV{ghprbPullId} - Commit: $ENV{ghprbActualCommit}")
else ()
  set(BUILD_SEQ "dev")
endif ()

if (WIN32)
  add_definitions(-D_USE_MATH_DEFINES) # apparently needed to get M_PI and other defines from cmath/math.h		
  add_definitions(-DWINDOWS_LEAN_AND_MEAN) # needed to make sure windows doesn't go to crazy with its defines		
endif()

configure_file(InterfaceVersion.h.in "${PROJECT_BINARY_DIR}/includes/InterfaceVersion.h")

macro(GroupSources curdir)
   file(GLOB children RELATIVE ${PROJECT_SOURCE_DIR}/${curdir} ${PROJECT_SOURCE_DIR}/${curdir}/*)
   foreach(child ${children})
      if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${curdir}/${child})
          GroupSources(${curdir}/${child})
      else()
          string(REPLACE "/" "\\" groupname ${curdir})
          source_group(${groupname} FILES ${PROJECT_SOURCE_DIR}/${curdir}/${child})
      endif()
   endforeach()
endmacro()

# grab the implementation and header files from src dirs
file(GLOB_RECURSE INTERFACE_SRCS "src/*.cpp" "src/*.h")
GroupSources("src")

# Add SpeechRecognizer if on Windows or OS X, otherwise remove
if (WIN32)
  # Use .cpp and .h files as is.
elseif (APPLE)
  file(GLOB INTERFACE_OBJCPP_SRCS "src/SpeechRecognizer.mm")
  set(INTERFACE_SRCS ${INTERFACE_SRCS} ${INTERFACE_OBJCPP_SRCS})
  get_filename_component(SPEECHRECOGNIZER_CPP "src/SpeechRecognizer.cpp" ABSOLUTE)
  list(REMOVE_ITEM INTERFACE_SRCS ${SPEECHRECOGNIZER_CPP})
else ()
  get_filename_component(SPEECHRECOGNIZER_H "src/SpeechRecognizer.h" ABSOLUTE)
  list(REMOVE_ITEM INTERFACE_SRCS ${SPEECHRECOGNIZER_H})
  get_filename_component(SPEECHRECOGNIZER_CPP "src/SpeechRecognizer.cpp" ABSOLUTE)
  list(REMOVE_ITEM INTERFACE_SRCS ${SPEECHRECOGNIZER_CPP})
endif ()

find_package(Qt5 COMPONENTS Gui Multimedia Network OpenGL Qml Quick Script Svg WebKitWidgets)

# grab the ui files in resources/ui
file (GLOB_RECURSE QT_UI_FILES ui/*.ui)
source_group("UI Files" FILES ${QT_UI_FILES})
# have qt5 wrap them and generate the appropriate header files
qt5_wrap_ui(QT_UI_HEADERS "${QT_UI_FILES}")

# add them to the interface source files
set(INTERFACE_SRCS ${INTERFACE_SRCS} "${QT_UI_HEADERS}" "${QT_RESOURCES}")

# translation disabled until we strip out the line numbers
# set(QM ${TARGET_NAME}_en.qm)
# set(TS ${TARGET_NAME}_en.ts)
# qt5_create_translation_custom(${QM} ${INTERFACE_SRCS} ${QT_UI_FILES} ${TS})

if (APPLE)
  # configure CMake to use a custom Info.plist
  SET_TARGET_PROPERTIES( ${this_target} PROPERTIES MACOSX_BUNDLE_INFO_PLIST MacOSXBundleInfo.plist.in )

  set(MACOSX_BUNDLE_BUNDLE_NAME Interface)
  set(MACOSX_BUNDLE_GUI_IDENTIFIER io.highfidelity.Interface)
  
  if (UPPER_CMAKE_BUILD_TYPE MATCHES RELEASE OR UPPER_CMAKE_BUILD_TYPE MATCHES RELWITHDEBINFO)
    set(ICON_FILENAME "interface.icns")
  else ()
    set(ICON_FILENAME "interface-beta.icns")
  endif ()
  
  # set how the icon shows up in the Info.plist file
  SET(MACOSX_BUNDLE_ICON_FILE "${ICON_FILENAME}")

  # set where in the bundle to put the resources file
  SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/icon/${ICON_FILENAME} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  
  set(DISCOVERED_RESOURCES "")

  # use the add_resources_to_os_x_bundle macro to recurse into resources
  add_resources_to_os_x_bundle("${CMAKE_CURRENT_SOURCE_DIR}/resources")

  # append the discovered resources to our list of interface sources
  list(APPEND INTERFACE_SRCS ${DISCOVERED_RESOURCES})
  
  set(INTERFACE_SRCS ${INTERFACE_SRCS} "${CMAKE_CURRENT_SOURCE_DIR}/icon/${ICON_FILENAME}")
endif()

# create the executable, make it a bundle on OS X
if (APPLE)
  add_executable(${TARGET_NAME} MACOSX_BUNDLE ${INTERFACE_SRCS} ${QM})
elseif(WIN32)
  add_executable(${TARGET_NAME} WIN32 ${INTERFACE_SRCS} ${QM})
else()
  add_executable(${TARGET_NAME} ${INTERFACE_SRCS} ${QM})
endif()

# set up the external glm library
add_dependency_external_projects(glm bullet)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PRIVATE ${GLM_INCLUDE_DIRS})

add_dependency_external_projects(LibOVR)
find_package(LibOVR REQUIRED)
target_include_directories(${TARGET_NAME} PRIVATE ${LIBOVR_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${LIBOVR_LIBRARIES})

find_package(Bullet REQUIRED)

# perform the system include hack for OS X to ignore warnings
if (APPLE)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${BULLET_INCLUDE_DIRS}")
else()
  target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${BULLET_INCLUDE_DIRS})
endif()

target_link_libraries(${TARGET_NAME} ${BULLET_LIBRARIES})

# link required hifi libraries
link_hifi_libraries(shared octree environment gpu model render fbx networking entities avatars
                    audio audio-client animation script-engine physics
                    render-utils entities-renderer ui auto-updater)

add_dependency_external_projects(sdl2)

# perform standard include and linking for found externals
foreach(EXTERNAL ${OPTIONAL_EXTERNALS})
  
  if (${${EXTERNAL}_UPPERCASE}_REQUIRED)
    find_package(${EXTERNAL} REQUIRED)
  else ()
    find_package(${EXTERNAL})
  endif ()
  
  if (${${EXTERNAL}_UPPERCASE}_FOUND AND NOT DISABLE_${${EXTERNAL}_UPPERCASE})
    add_definitions(-DHAVE_${${EXTERNAL}_UPPERCASE})
    
    # include the library directories (ignoring warnings)
    if (NOT ${${EXTERNAL}_UPPERCASE}_INCLUDE_DIRS)
      set(${${EXTERNAL}_UPPERCASE}_INCLUDE_DIRS ${${${EXTERNAL}_UPPERCASE}_INCLUDE_DIR})
    endif ()
    
    include_directories(SYSTEM ${${${EXTERNAL}_UPPERCASE}_INCLUDE_DIRS})
    
    # perform the system include hack for OS X to ignore warnings
    if (APPLE)
      foreach(EXTERNAL_INCLUDE_DIR  ${${${EXTERNAL}_UPPERCASE}_INCLUDE_DIRS})
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${EXTERNAL_INCLUDE_DIR}")
      endforeach()
    endif ()
    
    if (NOT ${${EXTERNAL}_UPPERCASE}_LIBRARIES)
      set(${${EXTERNAL}_UPPERCASE}_LIBRARIES ${${${EXTERNAL}_UPPERCASE}_LIBRARY})
    endif ()
    
    if (NOT APPLE OR NOT ${${EXTERNAL}_UPPERCASE} MATCHES "SIXENSE")
      target_link_libraries(${TARGET_NAME} ${${${EXTERNAL}_UPPERCASE}_LIBRARIES})
    elseif (APPLE AND NOT INSTALLER_BUILD)
      add_definitions(-DSIXENSE_LIB_FILENAME=\"${${${EXTERNAL}_UPPERCASE}_LIBRARY_RELEASE}\")
    endif ()
  endif ()
endforeach()

# special OS X modifications for RtMidi library
if (RTMIDI_FOUND AND NOT DISABLE_RTMIDI AND APPLE)
  find_library(CoreMIDI CoreMIDI)
  add_definitions(-D__MACOSX_CORE__)
  target_link_libraries(${TARGET_NAME} ${CoreMIDI})
endif ()

# include headers for interface and InterfaceConfig.
include_directories("${PROJECT_SOURCE_DIR}/src" "${PROJECT_BINARY_DIR}/includes")

target_link_libraries(
  ${TARGET_NAME}
  Qt5::Gui Qt5::Network Qt5::Multimedia Qt5::OpenGL Qt5::Script Qt5::Svg Qt5::WebKitWidgets
)

# assume we are using a Qt build without bearer management
add_definitions(-DQT_NO_BEARERMANAGEMENT)

if (APPLE)
  # link in required OS X frameworks and include the right GL headers
  find_library(OpenGL OpenGL)
  find_library(AppKit AppKit)

  target_link_libraries(${TARGET_NAME} ${OpenGL} ${AppKit})
    
  # install command for OS X bundle
  INSTALL(TARGETS ${TARGET_NAME}
    BUNDLE DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/install" COMPONENT Runtime
    RUNTIME DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/install" COMPONENT Runtime
  )
else (APPLE)
  # copy the resources files beside the executable
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
    "${PROJECT_SOURCE_DIR}/resources"
    $<TARGET_FILE_DIR:${TARGET_NAME}>/resources
  )
    
  find_package(OpenGL REQUIRED)
  
  if (${OPENGL_INCLUDE_DIR})
    include_directories(SYSTEM "${OPENGL_INCLUDE_DIR}")
  endif ()
  
  target_link_libraries(${TARGET_NAME} "${OPENGL_LIBRARY}")
  
  # link target to external libraries
  if (WIN32)
    add_dependency_external_projects(glew)
    find_package(GLEW REQUIRED)
    target_include_directories(${TARGET_NAME} PRIVATE ${GLEW_INCLUDE_DIRS})

    target_link_libraries(${TARGET_NAME} ${GLEW_LIBRARIES} wsock32.lib opengl32.lib Winmm.lib)

    if (USE_NSIGHT)
      # try to find the Nsight package and add it to the build if we find it
      find_package(NSIGHT)
      if (NSIGHT_FOUND)
        include_directories(${NSIGHT_INCLUDE_DIRS})
        add_definitions(-DNSIGHT_FOUND)
        target_link_libraries(${TARGET_NAME} "${NSIGHT_LIBRARIES}")
      endif ()
    endif()

  endif()
endif (APPLE)

copy_dlls_beside_windows_executable()
