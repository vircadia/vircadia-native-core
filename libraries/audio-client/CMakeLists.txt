set(TARGET_NAME audio-client)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Network)

link_hifi_libraries(audio)

# call macro to include our dependency includes and bubble them up via a property on our target
include_dependency_includes()

set(GVERB_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/gverb")

# As Gverb is currently the only reverb library, it's required.
find_package(Gverb REQUIRED)

if (GVERB_FOUND)
  file(GLOB GVERB_SRCS ${GVERB_SRC_DIRS}/*.c)
  include_directories(${GVERB_INCLUDE_DIRS})
  add_library(gverb STATIC ${GVERB_SRCS})
  target_link_libraries(${TARGET_NAME} gverb)
endif (GVERB_FOUND)