cmake_minimum_required(VERSION 2.8)

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")
set(GLM_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)
set(LODEPNG_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/LodePNG)
set(PORTAUDIO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/portaudio)

project(interface)

if (APPLE)
  # link in required OS X frameworks and include the right GL headers
  set(CMAKE_EXE_LINKER_FLAGS "-framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework CoreServices -framework Carbon -framework GLUT")
  set(GL_HEADERS "#include <GLUT/glut.h>\n#include <OpenGL/glext.h>")
else (APPLE)
  # include the right GL headers for UNIX
  set(GL_HEADERS "#include <GL/gl.h>\n#include <GL/glut.h>\n#include <GL/glext.h>")
endif (APPLE)

if (WIN32)
  set(GLUT_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external/glut)
  set(GL_HEADERS "#define GLEW_STATIC\n#define FREEGLUT_STATIC\n#define FREEGLUT_LIB_PRAGMAS 0\n#include <GL/glew.h>\n#include <GL/freeglut_std.h>\n#include <GL/freeglut_ext.h>")
endif (WIN32)

# create the InterfaceConfig.h file based on GL_HEADERS above
configure_file(InterfaceConfig.h.in ${PROJECT_BINARY_DIR}/includes/InterfaceConfig.h)

# grab the implementation and header files from src dir
file(GLOB INTERFACE_SRCS src/*.mm src/*.cpp src/*.h)

if (APPLE)
  # set how the icon shows up in the Info.plist file
  SET(MACOSX_BUNDLE_ICON_FILE interface.icns) 

  # grab the image and audio files
  FILE(GLOB INTERFACE_RES_DIRS resources/*)
  set(INTERFACE_RSRCS ${INTERFACE_RES_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/interface.icns)

  # set where in the bundle to put the resources file
  SET_SOURCE_FILES_PROPERTIES(${INTERFACE_RSRCS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  # include the icns file and resource files in the target
  SET(INTERFACE_SRCS ${INTERFACE_SRCS} ${INTERFACE_RSRCS})
endif (APPLE)

# create the executable, make it a bundle on OS X
add_executable(interface MACOSX_BUNDLE ${INTERFACE_SRCS})

# link in the hifi shared library
include(../LinkHifiShared.cmake)
link_hifi_shared_library(interface)

# find required libraries
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(GLM REQUIRED)
find_package(LodePNG REQUIRED)

# include headers for external libraries and InterfaceConfig.
include_directories(
    ${PROJECT_BINARY_DIR}/includes
    ${OPENGL_INCLUDE_DIR}
    ${GLUT_INCLUDE_DIR}
    ${GLM_INCLUDE_DIRS} 
    ${LODEPNG_INCLUDE_DIRS}
)

# link target to external libraries
if (WIN32)
  target_link_libraries(interface 
    ${OPENGL_LIBRARY}
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glut/Release/glew32.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glut/Release/freeglut.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glut/Release/pthread_lib.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glut/Release/lodepng.lib
    wsock32.lib
    )
else (WIN32)
  target_link_libraries(interface 
    ${OPENGL_LIBRARY}
    ${GLUT_LIBRARY}
    ${LODEPNG_LIBRARY}
    )
endif (WIN32)

if (NOT WIN32)
  # include PortAudio as external project
  include(ExternalProject)
  set(PORTAUDIO_PROJ_DIR external/portaudio)
  ExternalProject_Add(
      portaudio
      PREFIX ${PORTAUDIO_PROJ_DIR}
      BINARY_DIR ${PORTAUDIO_PROJ_DIR}/src/portaudio
      URL ${PORTAUDIO_DIR}/pa_snapshot_020813.tgz
      CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix "${PROJECT_BINARY_DIR}/${PORTAUDIO_PROJ_DIR}"
      BUILD_COMMAND make
  )

  # make PortAudio a dependency of the interface executable
  add_dependencies(interface portaudio)

  # include the PortAudio headers
  ExternalProject_Get_Property(portaudio source_dir)
  include_directories(${source_dir}/include)

  # link the PortAudio library
  ExternalProject_Get_Property(portaudio binary_dir)
  target_link_libraries(interface ${binary_dir}/lib/.libs/libportaudio.a)

endif (NOT WIN32)

# link required libraries on UNIX
if (UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    find_package(Librt REQUIRED)
    find_package(ALSA)
    find_package(Jack)
    
    target_link_libraries(interface
        ${CMAKE_THREAD_LIBS_INIT}
        ${LIBRT_LIBRARIES}
        ${JACK_LIBRARIES}
        ${ALSA_LIBRARIES}
    )
endif (UNIX AND NOT APPLE)

# install command for OS X bundle
INSTALL(TARGETS interface 
    BUNDLE DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/install COMPONENT Runtime
    RUNTIME DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/install COMPONENT Runtime
)