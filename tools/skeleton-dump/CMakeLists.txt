set(TARGET_NAME skeleton-dump)
setup_hifi_project(Core)
setup_memory_debugger()
setup_thread_debugger()
link_hifi_libraries(shared model-serializers hfm graphics gpu gl animation)

include_hifi_library_headers(image)
