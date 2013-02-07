cmake_minimum_required (VERSION 2.8)
project (interface)

add_subdirectory (src)

# third party libraries 
find_package (Portaudio REQUIRED)
include_directories (${Portaudio_INCLUDE_DIRS})
set (LIBS ${LIBS} ${Portaudio_LIBRARIES})

target_link_libraries (interface LIBS)