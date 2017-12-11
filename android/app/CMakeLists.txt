set(TARGET_NAME native-lib)
setup_hifi_library(Gui Qml Quick)
link_hifi_libraries(shared networking gl gpu gpu-gles image fbx render-utils physics entities octree audio midi recording controllers script-engine ui)
#link_hifi_libraries(shared networking gl ui)

target_link_libraries(native-lib android log m)
target_opengl()
target_bullet()