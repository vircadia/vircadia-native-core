set(TARGET_NAME procedural)
setup_hifi_library()
link_hifi_libraries(shared gpu shaders networking graphics model-networking ktx image)

include_hifi_library_headers(hfm)
