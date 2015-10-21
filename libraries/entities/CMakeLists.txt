set(TARGET_NAME entities)
setup_hifi_library(Network Script)
link_hifi_libraries(avatars shared octree gpu model fbx networking animation environment)

target_bullet()

include_hifi_library_headers(render)
