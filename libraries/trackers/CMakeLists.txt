set(TARGET_NAME trackers)
setup_hifi_library()
GroupSources("src")
link_hifi_libraries(shared)

target_bullet()
