cmake_minimum_required(VERSION 2.8)

if (WIN32)
  cmake_policy (SET CMP0020 NEW)
endif (WIN32)

set(ROOT_DIR ..)
set(MACRO_DIR "${ROOT_DIR}/cmake/macros")

set(TARGET_NAME interface)
project(${TARGET_NAME})

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules/")
set(FACEPLUS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/faceplus")
set(FACESHIFT_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/faceshift")
set(LIBOVR_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/oculus")
set(PRIOVR_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/priovr")
set(SIXENSE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/Sixense")
set(VISAGE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/visage")
set(LEAPMOTION_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/leapmotion")
set(RTMIDI_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/rtmidi")


find_package(Qt5LinguistTools REQUIRED)
find_package(Qt5LinguistToolsMacros)

if (DEFINED ENV{JOB_ID})
  set(BUILD_SEQ $ENV{JOB_ID})
else ()
  set(BUILD_SEQ "dev")
endif ()

if (APPLE)
  set(GL_HEADERS "#include <GLUT/glut.h>\n#include <OpenGL/glext.h>")
elseif (UNIX)
  # include the right GL headers for UNIX
  set(GL_HEADERS "#include <GL/gl.h>\n#include <GL/glut.h>\n#include <GL/glext.h>")
elseif (WIN32)
    add_definitions(-D_USE_MATH_DEFINES) # apparently needed to get M_PI and other defines from cmath/math.h
    add_definitions(-DWINDOWS_LEAN_AND_MEAN) # needed to make sure windows doesn't go to crazy with its defines

    set(GL_HEADERS "#include <windowshacks.h>\n#include <GL/glew.h>\n#include <GL/glut.h>")
endif ()

# set up the external glm library
include("${MACRO_DIR}/IncludeGLM.cmake")
include_glm(${TARGET_NAME} "${ROOT_DIR}")

# create the InterfaceConfig.h file based on GL_HEADERS above
configure_file(InterfaceConfig.h.in "${PROJECT_BINARY_DIR}/includes/InterfaceConfig.h")
configure_file(InterfaceVersion.h.in "${PROJECT_BINARY_DIR}/includes/InterfaceVersion.h")

# grab the implementation and header files from src dirs
file(GLOB INTERFACE_SRCS src/*.cpp src/*.h)
foreach(SUBDIR avatar devices renderer ui starfield location scripting voxels particles models)
  file(GLOB_RECURSE SUBDIR_SRCS src/${SUBDIR}/*.cpp src/${SUBDIR}/*.h)
  set(INTERFACE_SRCS ${INTERFACE_SRCS} "${SUBDIR_SRCS}")
endforeach(SUBDIR)

find_package(Qt5 COMPONENTS Core Gui Multimedia Network OpenGL Script Svg WebKit WebKitWidgets Xml UiTools)

# grab the ui files in resources/ui
file (GLOB_RECURSE QT_UI_FILES ui/*.ui)
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

  # set how the icon shows up in the Info.plist file
  SET(MACOSX_BUNDLE_ICON_FILE interface.icns)

  # set where in the bundle to put the resources file
  SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/interface.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  
  # grab the directories in resources and put them in the right spot in Resources
  file(GLOB RESOURCE_SUBDIRS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/resources" "${CMAKE_CURRENT_SOURCE_DIR}/resources/*")
  foreach(DIR ${RESOURCE_SUBDIRS})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources/${DIR}")
      FILE(GLOB DIR_CONTENTS "resources/${DIR}/*")
      SET_SOURCE_FILES_PROPERTIES(${DIR_CONTENTS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/${DIR}")

      SET(INTERFACE_SRCS ${INTERFACE_SRCS} "${DIR_CONTENTS}")
    endif()
  endforeach()

  SET(INTERFACE_SRCS ${INTERFACE_SRCS} "${CMAKE_CURRENT_SOURCE_DIR}/interface.icns")
endif()

# create the executable, make it a bundle on OS X
add_executable(${TARGET_NAME} MACOSX_BUNDLE ${INTERFACE_SRCS} ${QM})

# link in the hifi shared library
include(${MACRO_DIR}/LinkHifiLibrary.cmake)

# link required hifi libraries
link_hifi_library(shared ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(octree ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(voxels ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(fbx ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(metavoxels ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(networking ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(particles ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(models ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(avatars ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(audio ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(animation ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(script-engine ${TARGET_NAME} "${ROOT_DIR}")

# find any optional and required libraries
find_package(Faceplus)
find_package(Faceshift)
find_package(LibOVR)
find_package(PrioVR)
find_package(SDL)
find_package(Sixense)
find_package(Visage)
find_package(LeapMotion)
find_package(ZLIB)
find_package(Qxmpp)
find_package(RtMidi)
find_package(OpenSSL REQUIRED)

# include the Sixense library for Razer Hydra if available
if (SIXENSE_FOUND AND NOT DISABLE_SIXENSE)
  add_definitions(-DHAVE_SIXENSE)
  include_directories(SYSTEM "${SIXENSE_INCLUDE_DIRS}")
  if (APPLE OR UNIX)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${SIXENSE_INCLUDE_DIRS}")
  endif (APPLE OR UNIX)
  target_link_libraries(${TARGET_NAME} ${SIXENSE_LIBRARIES})
endif (SIXENSE_FOUND AND NOT DISABLE_SIXENSE)

# likewise with Visage library for webcam feature tracking
if (VISAGE_FOUND AND NOT DISABLE_VISAGE)
  add_definitions(-DHAVE_VISAGE -DVISAGE_STATIC)
  include_directories(SYSTEM "${VISAGE_INCLUDE_DIRS}")
  if (APPLE)
      add_definitions(-DMAC_OS_X)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-comment")
      include_directories(SYSTEM "${VISAGE_INCLUDE_DIRS}")
      find_library(AVFoundation AVFoundation)
      find_library(CoreMedia CoreMedia)
      find_library(NEW_STD_LIBRARY libc++.dylib /usr/lib/)
      target_link_libraries(${TARGET_NAME} ${AVFoundation} ${CoreMedia} ${NEW_STD_LIBRARY})
  endif (APPLE)
  target_link_libraries(${TARGET_NAME} "${VISAGE_LIBRARIES}")
endif (VISAGE_FOUND AND NOT DISABLE_VISAGE)

# and with Faceplus library, also for webcam feature tracking
if (FACEPLUS_FOUND AND NOT DISABLE_FACEPLUS)
  add_definitions(-DHAVE_FACEPLUS)
  include_directories(SYSTEM "${FACEPLUS_INCLUDE_DIRS}")
  target_link_libraries(${TARGET_NAME} "${FACEPLUS_LIBRARIES}")
endif (FACEPLUS_FOUND AND NOT DISABLE_FACEPLUS)

# and with LibOVR for Oculus Rift
if (LIBOVR_FOUND AND NOT DISABLE_LIBOVR)
  add_definitions(-DHAVE_LIBOVR)
  include_directories(SYSTEM ${LIBOVR_INCLUDE_DIRS})
  
  if (APPLE)
    foreach(LIBOVR_DIR ${LIBOVR_INCLUDE_DIRS})
       set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${LIBOVR_DIR}")
    endforeach()
  endif ()
  
  target_link_libraries(${TARGET_NAME} ${LIBOVR_LIBRARIES})
endif (LIBOVR_FOUND AND NOT DISABLE_LIBOVR)

# and with PrioVR library
if (PRIOVR_FOUND AND NOT DISABLE_PRIOVR)
  add_definitions(-DHAVE_PRIOVR)
  include_directories(SYSTEM "${PRIOVR_INCLUDE_DIRS}")
  target_link_libraries(${TARGET_NAME} "${PRIOVR_LIBRARIES}")
endif (PRIOVR_FOUND AND NOT DISABLE_PRIOVR)

# and with LeapMotion library
if (LEAPMOTION_FOUND AND NOT DISABLE_LEAPMOTION)
    add_definitions(-DHAVE_LEAPMOTION)
    include_directories(SYSTEM "${LEAPMOTION_INCLUDE_DIRS}")

    if (APPLE)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${LEAPMOTION_INCLUDE_DIRS}")
    endif ()
    target_link_libraries(${TARGET_NAME} ${LEAPMOTION_LIBRARIES})
endif (LEAPMOTION_FOUND AND NOT DISABLE_LEAPMOTION)

# and with SDL for joysticks
if (SDL_FOUND AND NOT DISABLE_SDL)
  add_definitions(-DHAVE_SDL)
  include_directories(SYSTEM "${SDL_INCLUDE_DIR}")
  target_link_libraries(${TARGET_NAME} "${SDL_LIBRARY}")
endif (SDL_FOUND AND NOT DISABLE_SDL)

# and with qxmpp for chat
if (QXMPP_FOUND AND NOT DISABLE_QXMPP)
  add_definitions(-DHAVE_QXMPP -DQXMPP_STATIC)
  include_directories(SYSTEM "${QXMPP_INCLUDE_DIRS}")

  target_link_libraries(${TARGET_NAME} ${QXMPP_LIBRARIES})
endif (QXMPP_FOUND AND NOT DISABLE_QXMPP)

# and with RtMidi for RtMidi control
if (RTMIDI_FOUND AND NOT DISABLE_RTMIDI)
  add_definitions(-DHAVE_RTMIDI)
  include_directories(SYSTEM "${RTMIDI_INCLUDE_DIRS}")
  target_link_libraries(${TARGET_NAME} ${RTMIDI_LIBRARIES})
  
  if (APPLE)
    find_library(CoreMIDI CoreMIDI)
    add_definitions(-D__MACOSX_CORE__)
    target_link_libraries(${TARGET_NAME} ${CoreMIDI})
  endif()
endif()

# and with Faceshift for depth camera face tracking
if (FACESHIFT_FOUND AND NOT DISABLE_FACESHIFT)
  add_definitions(-DHAVE_FACESHIFT)
  include_directories(SYSTEM "${FACESHIFT_INCLUDE_DIRS}")
  target_link_libraries(${TARGET_NAME} ${FACESHIFT_LIBRARIES})
endif()

# include headers for interface and InterfaceConfig.
include_directories("${PROJECT_SOURCE_DIR}/src" "${PROJECT_BINARY_DIR}/includes")

# include external library headers
# use system flag so warnings are supressed
include_directories(SYSTEM "${OPENSSL_INCLUDE_DIR}")

target_link_libraries(
  ${TARGET_NAME}
  "${ZLIB_LIBRARIES}"
  ${OPENSSL_LIBRARIES}
  Qt5::Core Qt5::Gui Qt5::Multimedia Qt5::Network Qt5::OpenGL 
  Qt5::Script Qt5::Svg Qt5::WebKit Qt5::WebKitWidgets Qt5::Xml Qt5::UiTools
)

# assume we are using a Qt build without bearer management
add_definitions(-DQT_NO_BEARERMANAGEMENT)

if (APPLE)
    # link in required OS X frameworks and include the right GL headers
    find_library(AppKit AppKit)
    find_library(CoreAudio CoreAudio)
    find_library(CoreServices CoreServices)
    find_library(Carbon Carbon)
    find_library(Foundation Foundation)
    find_library(GLUT GLUT)
    find_library(OpenGL OpenGL)
    find_library(IOKit IOKit)
    find_library(QTKit QTKit)
    find_library(QuartzCore QuartzCore)    

    target_link_libraries(
        ${TARGET_NAME}
        ${AppKit}
        ${CoreAudio}
        ${CoreServices}
        ${Carbon}
        ${Foundation}
        ${GLUT}
        ${OpenGL}
        ${IOKit}
        ${QTKit}
        ${QuartzCore}
    )
    
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
  find_package(GLUT REQUIRED)
  
  include_directories(SYSTEM "${GLUT_INCLUDE_DIRS}")
  
  if (${OPENGL_INCLUDE_DIR})
    include_directories(SYSTEM "${OPENGL_INCLUDE_DIR}")
  endif ()
  
  target_link_libraries(${TARGET_NAME} "${OPENGL_LIBRARY}" "${GLUT_LIBRARIES}")
  
  # link target to external libraries
  if (WIN32)
    find_package(GLEW REQUIRED)
    include_directories(${GLEW_INCLUDE_DIRS})
    
    # we're using static GLEW, so define GLEW_STATIC
    add_definitions(-DGLEW_STATIC)
    
    # add a definition for ssize_t so that windows doesn't bail
    add_definitions(-Dssize_t=long)
    
    target_link_libraries(${TARGET_NAME} "${GLEW_LIBRARIES}" wsock32.lib opengl32.lib)
  endif()
endif (APPLE)
