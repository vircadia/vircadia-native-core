set(TARGET_NAME script-engine)
# FIXME Move undo scripting interface to application and remove Widgets
setup_hifi_library(Network Script WebSockets)

target_zlib()
if (NOT ANDROID)
  target_quazip()
endif ()

link_hifi_libraries(networking)
include_hifi_library_headers(octree)
include_hifi_library_headers(shared)
