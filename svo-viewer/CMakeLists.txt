cmake_minimum_required(VERSION 2.8)

set(ROOT_DIR ..)
set(MACRO_DIR "${ROOT_DIR}/cmake/macros")

set(TARGET_NAME svo-viewer)
project(${TARGET_NAME})

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules/")

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
include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} "${ROOT_DIR}")

# create the ${TARGET_NAME}-config.h file based on GL_HEADERS above
configure_file(${TARGET_NAME}-config.h.in "${PROJECT_BINARY_DIR}/includes/${TARGET_NAME}-config.h")
configure_file(${TARGET_NAME}-version.h.in "${PROJECT_BINARY_DIR}/includes/${TARGET_NAME}-version.h")

# grab the implementation and header files from src dirs
file(GLOB APPLICATION_SRCS src/*.c src/*.cpp src/*.h)
foreach(SUBDIR avatar devices renderer ui starfield)
  file(GLOB_RECURSE SUBDIR_SRCS "src/${SUBDIR}/*.cpp" "src/${SUBDIR}/*.c" "src/${SUBDIR}/*.h")
  set(APPLICATION_SRCS ${APPLICATION_SRCS} "${SUBDIR_SRCS}")
endforeach(SUBDIR)

foreach(EXTERNAL_SOURCE_SUBDIR ${EXTERNAL_SOURCE_SUBDIRS})
  file(GLOB_RECURSE SUBDIR_SRCS 
    "external/${EXTERNAL_SOURCE_SUBDIR}/src/*.cpp" 
    "external/${EXTERNAL_SOURCE_SUBDIR}/src/*.c" 
    "external/${EXTERNAL_SOURCE_SUBDIR}/src/*.h")
  set(APPLICATION_SRCS ${APPLICATION_SRCS} "${SUBDIR_SRCS}")
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

# add them to the application source files
set(APPLICATION_SRCS ${APPLICATION_SRCS} "${QT_UI_HEADERS}")

if (APPLE)

  # configure CMake to use a custom Info.plist
  SET_TARGET_PROPERTIES( ${this_target} PROPERTIES MACOSX_BUNDLE_INFO_PLIST MacOSXBundleInfo.plist.in )

  set(MACOSX_BUNDLE_BUNDLE_NAME svo-viewer)
  set(MACOSX_BUNDLE_GUI_IDENTIFIER io.highfidelity.${TARGET_NAME})

  # set how the icon shows up in the Info.plist file
  SET(MACOSX_BUNDLE_ICON_FILE ${TARGET_NAME}.icns)

  # set where in the bundle to put the resources file
  SET_SOURCE_FILES_PROPERTIES("${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}.icns" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

  SET(APPLICATION_SRCS ${APPLICATION_SRCS} "${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}.icns")

  # grab the directories in resources and put them in the right spot in Resources
  file(GLOB RESOURCE_SUBDIRS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/resources" "${CMAKE_CURRENT_SOURCE_DIR}/resources/*")
  foreach(DIR ${RESOURCE_SUBDIRS})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources/${DIR}")
      FILE(GLOB DIR_CONTENTS "resources/${DIR}/*")
      SET_SOURCE_FILES_PROPERTIES(${DIR_CONTENTS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/${DIR}")

      SET(APPLICATION_SRCS ${APPLICATION_SRCS} "${DIR_CONTENTS}")
    endif()
  endforeach()
endif (APPLE)

# create the executable, make it a bundle on OS X
add_executable(${TARGET_NAME} MACOSX_BUNDLE ${APPLICATION_SRCS})

# link in the hifi shared library
include(${MACRO_DIR}/LinkHifiLibrary.cmake)

# link required hifi libraries
link_hifi_library(shared ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(octree ${TARGET_NAME} "${ROOT_DIR}")
link_hifi_library(voxels ${TARGET_NAME} "${ROOT_DIR}")

# find required libraries
find_package(GLM REQUIRED)
find_package(ZLIB)

qt5_use_modules(${TARGET_NAME} Core Gui Multimedia Network OpenGL Script Svg WebKit WebKitWidgets Xml UiTools)

# include headers for interface
include_directories("${PROJECT_SOURCE_DIR}/src" "${PROJECT_BINARY_DIR}/includes")

# include external library headers
# use system flag so warnings are supressed
include_directories(SYSTEM "${GLM_INCLUDE_DIRS}")

target_link_libraries(${TARGET_NAME} "${ZLIB_LIBRARIES}")

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
    include_directories("${GLUT_INCLUDE_DIR}" "${OPENGL_INCLUDE_DIR}")
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

