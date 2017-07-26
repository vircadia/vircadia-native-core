set(TARGET_NAME trackers)
setup_hifi_library()
GroupSources("src")
link_hifi_libraries(shared)
include_hifi_library_headers(octree)

target_bullet()
