cmake_minimum_required(VERSION 2.8)

project(interface)

if (APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework CoreServices -framework Carbon")
    set(GL_HEADERS "#include <GLUT/glut.h>\n#include <OpenGL/glext.h>")
else (APPLE)
    set(GL_HEADERS "#include <GL/gl.h>\n#include <GL/glut.h>\n#include <GL/glext.h>")
endif (APPLE)

configure_file(InterfaceConfig.h.in ${PROJECT_BINARY_DIR}/includes/InterfaceConfig.h)

file(GLOB INTERFACE_SRCS src/*.cpp src/*.h)
include_directories(${PROJECT_BINARY_DIR}/includes)

add_executable(interface ${INTERFACE_SRCS})

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(GLM REQUIRED)
find_package(Portaudio REQUIRED)
find_package(LodePNG REQUIRED)

include_directories(
    ${OPENGL_INCLUDE_DIRS}
    ${GLUT_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS} 
    ${PORTAUDIO_INCLUDE_DIRS} 
    ${LODEPNG_INCLUDE_DIRS}
)
target_link_libraries(interface 
    ${OPENGL_LIBRARY}
    ${GLUT_LIBRARY}
    ${PORTAUDIO_LIBRARY}
    ${LODEPNG_LIBRARY}
)