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
find_package(LodePNG REQUIRED)

include_directories(
    ${OPENGL_INCLUDE_DIRS}
    ${GLUT_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS} 
    ${LODEPNG_INCLUDE_DIRS}
)

target_link_libraries(interface 
    ${OPENGL_LIBRARY}
    ${GLUT_LIBRARY}
    ${LODEPNG_LIBRARY}
)

include(ExternalProject)
set(PORTAUDIO_PROJ_DIR external/portaudio)
ExternalProject_Add(
    portaudio
    PREFIX ${PORTAUDIO_PROJ_DIR}
    BINARY_DIR ${PORTAUDIO_PROJ_DIR}/src/portaudio
    URL ${PORTAUDIO_DIR}/pa_snapshot_020813.tgz
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix "${CMAKE_CURRENT_SOURCE_DIR}/${PORTAUDIO_PROJ_DIR}"
    BUILD_COMMAND make
)

if (UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    find_package(Librt REQUIRED)
    find_package(Jack REQUIRED)
    find_package(ALSA REQUIRED)
    
    target_link_libraries(portaudio
        ${CMAKE_THREAD_LIBS_INIT}
        ${LIBRT_LIBRARIES}
        ${JACK_LIBRARIES}
        ${ALSA_LIBRARIES}
    )
endif (UNIX AND NOT APPLE)

ExternalProject_Get_Property(portaudio binary_dir)
ExternalProject_Get_Property(portaudio source_dir)
include_directories(${source_dir}/include)
add_dependencies(interface portaudio)

target_link_libraries(interface ${binary_dir}/lib/.libs/libportaudio.a)