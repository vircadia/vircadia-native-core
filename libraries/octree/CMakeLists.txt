set(TARGET_NAME octree)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library()

include_glm()

link_hifi_libraries(shared networking)

# find ZLIB and OpenSSL
find_package(ZLIB)
find_package(OpenSSL REQUIRED)

include_directories(SYSTEM "${ZLIB_INCLUDE_DIRS}" "${OPENSSL_INCLUDE_DIR}")

# append ZLIB and OpenSSL to our list of libraries to link
list(APPEND ${TARGET_NAME}_LIBRARIES_TO_LINK "${ZLIB_LIBRARIES}")
list(APPEND ${TARGET_NAME}_LIBRARIES_TO_LINK "${OPENSSL_LIBRARIES}")

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies()