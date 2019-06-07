set(TARGET_NAME platform)

setup_hifi_library()
link_hifi_libraries(shared)

GroupSources("src")
target_json()
