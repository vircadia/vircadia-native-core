set(TARGET_NAME procedural)
setup_hifi_library()

link_hifi_libraries(shared shared-gui gpu shaders networking render graphics material-networking ktx image hfm)
