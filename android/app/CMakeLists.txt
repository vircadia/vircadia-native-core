set(TARGET_NAME native-lib)
setup_hifi_library()
link_hifi_libraries(shared networking gl gpu gpu-gles image fbx render-utils physics)

target_link_libraries(native-lib android log m)

target_opengl()
target_googlevr()


