set(TARGET_NAME entities)
setup_hifi_library(Network Script)
link_hifi_libraries(avatars shared audio octree model model-networking fbx networking animation)
include_hifi_library_headers(networking)
include_hifi_library_headers(gpu)

target_bullet()

include_hifi_library_headers(render)
