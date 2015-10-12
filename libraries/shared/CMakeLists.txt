set(TARGET_NAME shared)

# TODO: there isn't really a good reason to have Script linked here - let's get what is requiring it out (RegisteredMetaTypes.cpp)
setup_hifi_library(Gui Network Script Widgets)

find_package(ZLIB REQUIRED)
target_link_libraries(${TARGET_NAME} ${ZLIB_LIBRARIES})
target_include_directories(${TARGET_NAME} PUBLIC ${ZLIB_INCLUDE_DIRS})

if (WIN32)
  # Birarda will fix this when he finds it.
  get_filename_component(ZLIB_LIB_DIR "${ZLIB_LIBRARIES}" DIRECTORY)
  get_filename_component(ZLIB_DIR "${ZLIB_LIB_DIR}" DIRECTORY)
  set(ZLIB_BIN_DIR "${ZLIB_DIR}/bin")
  add_paths_to_fixup_libs(${ZLIB_BIN_DIR})
endif ()
