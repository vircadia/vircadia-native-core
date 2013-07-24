cmake_minimum_required(VERSION 2.8)

project(hifi)

set(CMAKE_PREFIX_PATH "/usr/local/Cellar/qt5/5.1.0/lib/cmake")

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

add_subdirectory(animation-server)
add_subdirectory(avatar-mixer)
add_subdirectory(audio-mixer)
add_subdirectory(domain-server)
add_subdirectory(eve)
add_subdirectory(interface)
add_subdirectory(injector)
add_subdirectory(pairing-server)
add_subdirectory(space-server)
add_subdirectory(voxel-edit)
add_subdirectory(voxel-server)