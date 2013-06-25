cmake_minimum_required(VERSION 2.8)
include_directories(include)

# grab the implementation and header files from src dirs
file(GLOB SKELTRACK_SRCS src/*.c include/*.h)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 glib-2.0)

string(REPLACE ";" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GLIB2_STATIC_CFLAGS} ${GLIB2_STATIC_LDFLAGS}")
message("${CMAKE_C_FLAGS}")

add_library(skeltrack ${SKELTRACK_SRCS})

