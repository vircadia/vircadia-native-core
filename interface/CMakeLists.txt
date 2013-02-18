cmake_minimum_required(VERSION 2.8)

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")
set(GLM_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)
set(LODEPNG_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/LodePNG)
set(PORTAUDIO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/portaudio)

project(interface)

if (APPLE)
    # link in required OS X frameworks and include the right GL headers
    set(CMAKE_EXE_LINKER_FLAGS "-framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework CoreServices -framework Carbon")
    set(GL_HEADERS "#include <GLUT/glut.h>\n#include <OpenGL/glext.h>")
else (APPLE)
    # include the right GL headers for UNIX
    set(GL_HEADERS "#include <GL/gl.h>\n#include <GL/glut.h>\n#include <GL/glext.h>")
endif (APPLE)

# create the InterfaceConfig.h file based on GL_HEADERS above
configure_file(InterfaceConfig.h.in ${PROJECT_BINARY_DIR}/includes/InterfaceConfig.h)

# grab the implementation and header files from src dir
file(GLOB INTERFACE_SRCS src/*.cpp src/*.h)

IF(APPLE)
  # set how the icon shows up in the Info.plist file
  SET(MACOSX_BUNDLE_ICON_FILE interface.icns) 
  # set where in the bundle to put the icns file
  SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/interface.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  # include the icns file in the target
  SET(INTERFACE_SRCS ${INTERFACE_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/interface.icns)
ENDIF(APPLE)

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
    ${OPENGL_INCLUDE_DIRS}
    ${GLUT_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS} 
    ${LODEPNG_INCLUDE_DIRS}
)

# link target to external libraries
target_link_libraries(interface 
    ${OPENGL_LIBRARY}
    ${GLUT_LIBRARY}
    ${LODEPNG_LIBRARY}
)

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