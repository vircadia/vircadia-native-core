cmake_minimum_required(VERSION 2.8)

set(ROOT_DIR ..)
set(MACRO_DIR ${ROOT_DIR}/cmake/macros)

set(TARGET_NAME interface)
project(${TARGET_NAME})

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules/")
set(LIBOVR_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/LibOVR)
set(LIBVPX_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/LibVPX)
set(LEAP_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/Leap)
set(MOTIONDRIVER_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/MotionDriver)
set(PORTAUDIO_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/PortAudio)
set(OPENCV_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/OpenCV)
set(UVCCAMERACONTROL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/UVCCameraControl)

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

# grab the implementation and header files from src dirs
file(GLOB INTERFACE_SRCS src/*.cpp src/*.h)
foreach(SUBDIR avatar ui renderer)
  file(GLOB SUBDIR_SRCS src/${SUBDIR}/*.cpp src/${SUBDIR}/*.h)
  set(INTERFACE_SRCS ${INTERFACE_SRCS} ${SUBDIR_SRCS})
endforeach(SUBDIR)

# project subdirectories
add_subdirectory(src/starfield)

if (APPLE)
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

find_package(Qt4 REQUIRED QtCore QtGui QtNetwork QtOpenGL QtWebKit QtSvg)
include(${QT_USE_FILE})
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${QT_QTGUI_INCLUDE_DIR}")

set(QUAZIP_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/quazip)
add_subdirectory(external/fervor/)
include_directories(external/fervor/)

# run qt moc on qt-enabled headers
qt4_wrap_cpp(INTERFACE_SRCS src/Application.h src/Webcam.h src/avatar/AvatarVoxelSystem.h
    src/avatar/Face.h src/ui/BandwidthDialog.h)

# create the executable, make it a bundle on OS X
add_executable(${TARGET_NAME} MACOSX_BUNDLE ${INTERFACE_SRCS})

# link in the hifi shared library
include(${MACRO_DIR}/LinkHifiLibrary.cmake)

# link required hifi libraries
link_hifi_library(shared ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(voxels ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(avatars ${TARGET_NAME} ${ROOT_DIR})
link_hifi_library(audio ${TARGET_NAME} ${ROOT_DIR})

# find required libraries
find_package(GLM REQUIRED)
find_package(LibOVR)
find_package(LibVPX)
find_package(Leap)
find_package(MotionDriver)
find_package(OpenCV)
find_package(OpenNI)
find_package(UVCCameraControl)
find_package(ZLIB)

# let the source know that we have OpenNI/NITE for Kinect
if (OPENNI_FOUND)
    add_definitions(-DHAVE_OPENNI)
    include_directories(SYSTEM ${OPENNI_INCLUDE_DIRS})
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${OPENNI_INCLUDE_DIRS}")
    target_link_libraries(${TARGET_NAME} ${OPENNI_LIBRARIES})
endif (OPENNI_FOUND)

# include headers for interface and InterfaceConfig.
include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/includes
)

# include external library headers
# use system flag so warnings are supressed
include_directories(
  SYSTEM
  ${GLM_INCLUDE_DIRS} 
  ${LIBOVR_INCLUDE_DIRS}
  ${LIBVPX_INCLUDE_DIRS}
  ${LEAP_INCLUDE_DIRS}
  ${MOTIONDRIVER_INCLUDE_DIRS}
  ${OPENCV_INCLUDE_DIRS}
)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${OPENCV_INCLUDE_DIRS}")
target_link_libraries(
  ${TARGET_NAME} 
  ${QT_LIBRARIES}
  ${LIBVPX_LIBRARIES}
  ${MOTIONDRIVER_LIBRARIES}
  ${OPENCV_LIBRARIES} 
  ${ZLIB_LIBRARIES}
  fervor
)

if (APPLE)
    # link in required OS X frameworks and include the right GL headers
    find_library(AppKit AppKit)
    find_library(AudioToolbox AudioToolbox)
    find_library(AudioUnit AudioUnit)
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
        ${AudioToolbox} 
        ${AudioUnit} 
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
        ${LIBOVR_LIBRARIES}
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
  # link the PortAudio library
  find_package(PortAudio REQUIRED)
  include_directories(${PORTAUDIO_INCLUDE_DIRS})
  target_link_libraries(${TARGET_NAME} ${PORTAUDIO_LIBRARIES})
  
  # link required libraries on UNIX
  if (UNIX AND NOT APPLE) 
      find_package(Threads REQUIRED)
      find_package(Librt REQUIRED)
      find_package(ALSA)
      find_package(Jack)

      target_link_libraries(${TARGET_NAME}
          ${CMAKE_THREAD_LIBS_INIT}
          ${LIBRT_LIBRARIES}
          ${JACK_LIBRARIES}
          ${ALSA_LIBRARIES}
          ${GLUT_LIBRARY}
      )
  endif (UNIX AND NOT APPLE)
endif (WIN32)

# install command for OS X bundle
INSTALL(TARGETS ${TARGET_NAME} 
    BUNDLE DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/install COMPONENT Runtime
    RUNTIME DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/install COMPONENT Runtime
)
