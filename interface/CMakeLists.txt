cmake_minimum_required(VERSION 2.8)

set(ROOT_DIR ..)
set(MACRO_DIR "${ROOT_DIR}/cmake/macros")

set(TARGET_NAME interface)
project(${TARGET_NAME})

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules/")
set(FACESHIFT_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/faceshift")
set(LIBOVR_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/LibOVR")
set(SIXENSE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/Sixense")
set(VISAGE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/visage")

if (DEFINED ENV{JOB_ID})
  set(BUILD_SEQ $ENV{JOB_ID})
else ()
  set(BUILD_SEQ "dev")
endif ()

if (APPLE)
  set(GL_HEADERS "#include <GLUT/glut.h>\n#include <OpenGL/glext.h>")
endif (APPLE)

if (UNIX AND NOT APPLE)
  # include the right GL headers for UNIX
  set(GL_HEADERS "#include <GL/gl.h>\n#include <GL/glut.h>\n#include <GL/glext.h>")
endif (UNIX AND NOT APPLE)

if (WIN32)

    add_definitions( -D_USE_MATH_DEFINES ) # apparently needed to get M_PI and other defines from cmath/math.h
    add_definitions( -DWINDOWS_LEAN_AND_MEAN ) # needed to make sure windows doesn't go to crazy with its defines

    # windows build needs an external glut, we're using freeglut
    set(GLUT_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/freeglut")
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${GLUT_ROOT_PATH}")

    # windows build needs glew (opengl extention wrangler) this will handle providing access to OpenGL methods after 1.1
    # which are not accessible on windows without glew or some other dynamic linking mechanism
    set(GLEW_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/glew")
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${GLEW_ROOT_PATH}")
    include_directories(SYSTEM "${GLEW_ROOT_PATH}/include" "${GLUT_ROOT_PATH}/include")

    #set(GL_HEADERS "#define GLEW_STATIC\n#define FREEGLUT_STATIC\n#define FREEGLUT_LIB_PRAGMAS 0\n#include <GL/glew.h>\n#include <GL/wglew.h>\n#include <GL/freeglut_std.h>\n#include <GL/freeglut_ext.h>")
    set(GL_HEADERS "#define GLEW_STATIC\n#include <windowshacks.h>\n#include <GL/glew.h>\n#include <GL/glut.h>")

endif (WIN32)

# set up the external glm library
include("${MACRO_DIR}/IncludeGLM.cmake")
include_glm(${TARGET_NAME} "${ROOT_DIR}")

# create the InterfaceConfig.h file based on GL_HEADERS above
configure_file(InterfaceConfig.h.in "${PROJECT_BINARY_DIR}/includes/InterfaceConfig.h")
configure_file(InterfaceVersion.h.in "${PROJECT_BINARY_DIR}/includes/InterfaceVersion.h")

# grab the implementation and header files from src dirs
file(GLOB INTERFACE_SRCS src/*.cpp src/*.h)
foreach(SUBDIR avatar devices renderer ui starfield)
  file(GLOB_RECURSE SUBDIR_SRCS src/${SUBDIR}/*.cpp src/${SUBDIR}/*.h)
  set(INTERFACE_SRCS ${INTERFACE_SRCS} "${SUBDIR_SRCS}")
endforeach(SUBDIR)

#windows also includes the faceshift externals, because using a lib doesn't work due to debug/release mismatch
if (WIN32)
  set(EXTERNAL_SOURCE_SUBDIRS "faceshift")
endif (WIN32)
foreach(EXTERNAL_SOURCE_SUBDIR ${EXTERNAL_SOURCE_SUBDIRS})
  file(GLOB_RECURSE SUBDIR_SRCS 
    "external/${EXTERNAL_SOURCE_SUBDIR}/src/*.cpp" 
    "external/${EXTERNAL_SOURCE_SUBDIR}/src/*.c" 
    "external/${EXTERNAL_SOURCE_SUBDIR}/src/*.h")
  set(INTERFACE_SRCS ${INTERFACE_SRCS} "${SUBDIR_SRCS}")
endforeach(EXTERNAL_SOURCE_SUBDIR)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Multimedia REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(Qt5Svg REQUIRED)
find_package(Qt5WebKit REQUIRED)
find_package(Qt5WebKitWidgets REQUIRED)
find_package(Qt5Xml REQUIRED)

# grab the ui files in resources/ui
file (GLOB_RECURSE QT_UI_FILES ui/*.ui)
# have qt5 wrap them and generate the appropriate header files
qt5_wrap_ui(QT_UI_HEADERS "${QT_UI_FILES}")

# add them to the interface source files
set(INTERFACE_SRCS ${INTERFACE_SRCS} "${QT_UI_HEADERS}")

if (APPLE)

  # configure CMake to use a custom Info.plist
  SET_TARGET_PROPERTIES( ${this_target} PROPERTIES MACOSX_BUNDLE_INFO_PLIST MacOSXBundleInfo.plist.in )

  set(MACOSX_BUNDLE_BUNDLE_NAME Interface)
  set(MACOSX_BUNDLE_GUI_IDENTIFIER io.highfidelity.Interface)

  # set how the icon shows up in the Info.plist file
  SET(MACOSX_BUNDLE_ICON_FILE interface.icns)

  # set where in the bundle to put the resources file
  SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/interface.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

  SET(INTERFACE_SRCS ${INTERFACE_SRCS} "${CMAKE_CURRENT_SOURCE_DIR}/interface.icns")

  # grab the directories in resources and put them in the right spot in Resources
  file(GLOB RESOURCE_SUBDIRS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/resources" "${CMAKE_CURRENT_SOURCE_DIR}/resources/*")
  foreach(DIR ${RESOURCE_SUBDIRS})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources/${DIR}")
      FILE(GLOB DIR_CONTENTS "resources/${DIR}/*")
      SET_SOURCE_FILES_PROPERTIES(${DIR_CONTENTS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/${DIR}")

      SET(INTERFACE_SRCS ${INTERFACE_SRCS} "${DIR_CONTENTS}")
    endif()
  endforeach()
endif (APPLE)

# create the executable, make it a bundle on OS X
add_executable(${TARGET_NAME} MACOSX_BUNDLE ${INTERFACE_SRCS})

# link in the hifi shared library
include(${MACRO_DIR}/LinkHifiLibrary.cmake)

# link required hifi libraries
link_hifi_library(shared ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(octree ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(voxels ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(metavoxels ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(particles ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(avatars ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(audio ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(script-engine ${TARGET_NAME} "${ROOT_DIR}")

# find required libraries
find_package(Faceshift)
find_package(GLM REQUIRED)
find_package(LibOVR)
find_package(Sixense)
find_package(Visage)
find_package(ZLIB)

# include the Sixense library for Razer Hydra if available
if (SIXENSE_FOUND AND NOT DISABLE_SIXENSE)
    add_definitions(-DHAVE_SIXENSE)
    include_directories(SYSTEM "${SIXENSE_INCLUDE_DIRS}")
    if (APPLE OR UNIX)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${SIXENSE_INCLUDE_DIRS}")
    endif (APPLE OR UNIX)
    target_link_libraries(${TARGET_NAME} "${SIXENSE_LIBRARIES}")
endif (SIXENSE_FOUND AND NOT DISABLE_SIXENSE)

# likewise with Visage library for webcam feature tracking
if (VISAGE_FOUND AND NOT DISABLE_VISAGE)
    add_definitions(-DHAVE_VISAGE -DVISAGE_STATIC)
    include_directories(SYSTEM "${VISAGE_INCLUDE_DIRS}")
    if (APPLE)
        add_definitions(-DMAC_OS_X)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-comment -isystem ${VISAGE_INCLUDE_DIRS}")
        find_library(AVFoundation AVFoundation)
        find_library(CoreMedia CoreMedia)
        find_library(NEW_STD_LIBRARY libc++.dylib /usr/lib/)
        target_link_libraries(${TARGET_NAME} ${AVFoundation} ${CoreMedia} ${NEW_STD_LIBRARY})
    endif (APPLE)
    target_link_libraries(${TARGET_NAME} "${VISAGE_LIBRARIES}")
endif (VISAGE_FOUND AND NOT DISABLE_VISAGE)

# and with LibOVR for Oculus Rift
if (LIBOVR_FOUND AND NOT DISABLE_LIBOVR)
    add_definitions(-DHAVE_LIBOVR)
    include_directories(SYSTEM "${LIBOVR_INCLUDE_DIRS}")
    if (APPLE OR UNIX)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${LIBOVR_INCLUDE_DIRS}")
    endif (APPLE OR UNIX)
    target_link_libraries(${TARGET_NAME} "${LIBOVR_LIBRARIES}")
endif (LIBOVR_FOUND AND NOT DISABLE_LIBOVR)

qt5_use_modules(${TARGET_NAME} Core Gui Multimedia Network OpenGL Script Svg WebKit WebKitWidgets Xml UiTools)

# include headers for interface and InterfaceConfig.
include_directories(
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_BINARY_DIR}/includes"
)

# include external library headers
# use system flag so warnings are supressed
include_directories(
  SYSTEM
  "${FACESHIFT_INCLUDE_DIRS}"
  "${GLM_INCLUDE_DIRS}"
)

target_link_libraries(
  ${TARGET_NAME}
  "${FACESHIFT_LIBRARIES}"
  "${ZLIB_LIBRARIES}"
)

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
else (APPLE)
    find_package(OpenGL REQUIRED)
    find_package(GLUT REQUIRED)
    include_directories(${GLUT_INCLUDE_DIR} "${OPENGL_INCLUDE_DIR}")
    target_link_libraries(${TARGET_NAME} "${OPENGL_LIBRARY}")
endif (APPLE)

# link target to external libraries
if (WIN32)
    target_link_libraries(
    ${TARGET_NAME}

    "${CMAKE_CURRENT_SOURCE_DIR}/external/glew/lib/Release/Win32/glew32s.lib"
    "${GLUT_ROOT_PATH}/lib/freeglut.lib"

    wsock32.lib
    opengl32.lib
    )
else (WIN32)
  # link required libraries on UNIX
  if (UNIX AND NOT APPLE)
      find_package(Threads REQUIRED)

      target_link_libraries(
        ${TARGET_NAME}
        "${CMAKE_THREAD_LIBS_INIT}"
        "${GLUT_LIBRARY}"
      )
  endif (UNIX AND NOT APPLE)
endif (WIN32)

# install command for OS X bundle
INSTALL(TARGETS ${TARGET_NAME}
    BUNDLE DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/install" COMPONENT Runtime
    RUNTIME DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/install" COMPONENT Runtime
)

