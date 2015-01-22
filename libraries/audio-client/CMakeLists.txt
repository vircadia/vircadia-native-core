set(TARGET_NAME audio-client)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Network Multimedia)

link_hifi_libraries(audio)

# append audio includes to our list of includes to bubble
list(APPEND ${TARGET_NAME}_DEPENDENCY_INCLUDES "${HIFI_LIBRARY_DIR}/audio/src")

set(GVERB_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/gverb")

# As Gverb is currently the only reverb library, it's required.
find_package(Gverb REQUIRED)

file(GLOB GVERB_SRCS ${GVERB_SRC_DIRS}/*.c)
add_library(gverb STATIC ${GVERB_SRCS})
target_link_libraries(${TARGET_NAME} gverb)

# append gverb includes to our list of includes to bubble
list(APPEND ${TARGET_NAME}_DEPENDENCY_INCLUDES "${GVERB_INCLUDE_DIRS}")

# we use libsoxr for resampling
find_package(LibSoxr REQUIRED)
target_link_libraries(${TARGET_NAME} ${LIBSOXR_LIBRARIES})
include_directories(SYSTEM ${LIBSOXR_INCLUDE_DIRS})

# call macro to include our dependency includes and bubble them up via a property on our target
include_dependency_includes()