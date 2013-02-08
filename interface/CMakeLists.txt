cmake_minimum_required(VERSION 2.8)

project(interface)

if (APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework CoreServices -framework Carbon")
    set(GLUT_HEADER_INCLUDE "<GLUT/glut.h>")
else (APPLE)
    set(GLUT_HEADER_INCLUDE "<GL/glut.h>")
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