cmake_minimum_required(VERSION 2.8)

set(ROOT_DIR ..)
set(MACRO_DIR ${ROOT_DIR}/cmake/macros)

set(TARGET_NAME interface)
project(${TARGET_NAME})

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules/")
set(FACESHIFT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/faceshift)
set(LIBOVR_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/LibOVR)
set(LIBVPX_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/LibVPX)
set(LEAP_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/Leap)
set(MOTIONDRIVER_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/MotionDriver)
set(OPENCV_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/OpenCV)
set(SIXENSE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/Sixense)
set(UVCCAMERACONTROL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/UVCCameraControl)

if (DEFINED ENV{JOB_ID})
  set(BUILD_SEQ $ENV{JOB_ID})
else ()
  set(BUILD_SEQ "0")
endif ()

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} $ENV{QT_CMAKE_PREFIX_PATH})

if (APPLE)
  set(GL_HEADERS "#include <GLUT/glut.h>\n#include <OpenGL/glext.h>")
else (APPLE)
  # include the right GL headers for UNIX
  set(GL_HEADERS "#include <GL/gl.h>\n#include <GL/glut.h>\n#include <GL/glext.h>")
endif (APPLE)

if (WIN32)
  set(GLUT_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external/glut)
  set(GL_HEADERS "#define GLEW_STATIC\n#define FREEGLUT_STATIC\n#define FREEGLUT_LIB_PRAGMAS 0\n#include <GL/glew.h>\n#include <GL/wglew.h>\n#include <GL/freeglut_std.h>\n#include <GL/freeglut_ext.h>")
endif (WIN32)

# set up the external glm library
include(${MACRO_DIR}/IncludeGLM.cmake)
include_glm(${TARGET_NAME} ${ROOT_DIR})

# create the InterfaceConfig.h file based on GL_HEADERS above
configure_file(InterfaceConfig.h.in ${PROJECT_BINARY_DIR}/includes/InterfaceConfig.h)
configure_file(InterfaceVersion.h.in ${PROJECT_BINARY_DIR}/includes/InterfaceVersion.h)

# grab the implementation and header files from src dirs
file(GLOB INTERFACE_SRCS src/*.cpp src/*.h)
foreach(SUBDIR avatar devices renderer ui starfield)
  file(GLOB_RECURSE SUBDIR_SRCS src/${SUBDIR}/*.cpp src/${SUBDIR}/*.h)
  set(INTERFACE_SRCS ${INTERFACE_SRCS} ${SUBDIR_SRCS})
endforeach(SUBDIR)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Multimedia REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(Qt5Svg REQUIRED)
find_package(Qt5WebKit REQUIRED)
find_package(Qt5WebKitWidgets REQUIRED)

if (APPLE)
  set(MACOSX_BUNDLE_BUNDLE_NAME Interface)
  # set how the icon shows up in the Info.plist file
  SET(MACOSX_BUNDLE_ICON_FILE interface.icns) 
  
  # set where in the bundle to put the resources file
  SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/interface.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  
  SET(INTERFACE_SRCS ${INTERFACE_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/interface.icns)
  
  # grab the directories in resources and put them in the right spot in Resources
  file(GLOB RESOURCE_SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/resources ${CMAKE_CURRENT_SOURCE_DIR}/resources/*)
  foreach(DIR ${RESOURCE_SUBDIRS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/${DIR})
      FILE(GLOB DIR_CONTENTS resources/${DIR}/*)
      SET_SOURCE_FILES_PROPERTIES(${DIR_CONTENTS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/${DIR})
    
      SET(INTERFACE_SRCS ${INTERFACE_SRCS} ${DIR_CONTENTS})
    endif()    
  endforeach()  
endif (APPLE)

# create the executable, make it a bundle on OS X
add_executable(${TARGET_NAME} MACOSX_BUNDLE ${INTERFACE_SRCS})

# link in the hifi shared library
include(${MACRO_DIR}/LinkHifiLibrary.cmake)

# link required hifi libraries
link_hifi_library(shared ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(octree ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(voxels ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(metavoxels ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(particles ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(avatars ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(audio ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(script-engine ${TARGET_NAME} ${ROOT_DIR})

# find required libraries
find_package(Faceshift)
find_package(GLM REQUIRED)
find_package(LibOVR)
find_package(LibVPX)
find_package(Leap)
find_package(MotionDriver)
find_package(OpenCV)
find_package(OpenNI)
find_package(Sixense)
find_package(UVCCameraControl)
find_package(ZLIB)

# let the source know that we have OpenNI/NITE for Kinect
if (OPENNI_FOUND AND NOT DISABLE_OPENNI)
    add_definitions(-DHAVE_OPENNI)
    include_directories(SYSTEM ${OPENNI_INCLUDE_DIRS})
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${OPENNI_INCLUDE_DIRS}")
    target_link_libraries(${TARGET_NAME} ${OPENNI_LIBRARIES})
endif (OPENNI_FOUND AND NOT DISABLE_OPENNI)

# likewise with Sixense library for Razer Hydra
if (SIXENSE_FOUND AND NOT DISABLE_SIXENSE)
    add_definitions(-DHAVE_SIXENSE)
    include_directories(SYSTEM ${SIXENSE_INCLUDE_DIRS})
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${SIXENSE_INCLUDE_DIRS}")
    target_link_libraries(${TARGET_NAME} ${SIXENSE_LIBRARIES})
endif (SIXENSE_FOUND AND NOT DISABLE_SIXENSE)

# and with LibOVR for Oculus Rift
if (LIBOVR_FOUND AND NOT DISABLE_LIBOVR)
    add_definitions(-DHAVE_LIBOVR)
    include_directories(SYSTEM ${LIBOVR_INCLUDE_DIRS})
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${LIBOVR_INCLUDE_DIRS}")
    target_link_libraries(${TARGET_NAME} ${LIBOVR_LIBRARIES})
endif (LIBOVR_FOUND AND NOT DISABLE_LIBOVR)

qt5_use_modules(${TARGET_NAME} Core Gui Multimedia Network OpenGL Script Svg WebKit WebKitWidgets)

# include headers for interface and InterfaceConfig.
include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/includes
)

# include external library headers
# use system flag so warnings are supressed
include_directories(
  SYSTEM
  ${FACESHIFT_INCLUDE_DIRS} 
  ${GLM_INCLUDE_DIRS} 
  ${LIBVPX_INCLUDE_DIRS}
  ${LEAP_INCLUDE_DIRS}
  ${MOTIONDRIVER_INCLUDE_DIRS}
  ${OPENCV_INCLUDE_DIRS}
)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${OPENCV_INCLUDE_DIRS}")
target_link_libraries(
  ${TARGET_NAME}
  ${FACESHIFT_LIBRARIES}
  ${LIBVPX_LIBRARIES}
  ${MOTIONDRIVER_LIBRARIES}
  ${OPENCV_LIBRARIES} 
  ${ZLIB_LIBRARIES}
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
    
    include_directories(SYSTEM ${UVCCAMERACONTROL_INCLUDE_DIRS})
    
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
        ${UVCCAMERACONTROL_LIBRARIES}
    )
    
    if (LEAP_FOUND)
      target_link_libraries(${TARGET_NAME} ${LEAP_LIBRARIES})
    endif(LEAP_FOUND)
    
else (APPLE)
    find_package(OpenGL REQUIRED)
    find_package(GLUT REQUIRED)
    include_directories(${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
    target_link_libraries(${TARGET_NAME} ${OPENGL_LIBRARY})
endif (APPLE)

# link target to external libraries
if (WIN32)
    target_link_libraries(
        ${TARGET_NAME} 
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glut/Release/glew32.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glut/Release/freeglut.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glut/Release/pthread_lib.lib
        wsock32.lib
    )
else (WIN32)
  # link required libraries on UNIX
  if (UNIX AND NOT APPLE) 
      find_package(Threads REQUIRED)

      target_link_libraries(
        ${TARGET_NAME}
        ${CMAKE_THREAD_LIBS_INIT}
        ${GLUT_LIBRARY}
      )
  endif (UNIX AND NOT APPLE)
endif (WIN32)

# install command for OS X bundle
INSTALL(TARGETS ${TARGET_NAME} 
    BUNDLE DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/install COMPONENT Runtime
    RUNTIME DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/install COMPONENT Runtime
)
