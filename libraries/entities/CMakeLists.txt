set(TARGET_NAME entities)
setup_hifi_library(Network Script)
include_directories(SYSTEM "${OPENSSL_INCLUDE_DIR}")
link_hifi_libraries(shared networking octree avatars model)
