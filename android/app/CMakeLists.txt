set(TARGET_NAME native-lib)
setup_hifi_library(Gui Qml Quick)

# Minimal dependencies for testing UI compositing
#link_hifi_libraries(shared networking gl ui)

link_hifi_libraries(
    shared networking octree
    script-engine recording trackers
    gl ktx image gpu gpu-gles render render-utils
    physics
    audio audio-client
    ui midi controllers pointers
    graphics model-networking fbx animation
    entities entities-renderer
    avatars avatars-renderer
    ui-plugins input-plugins
    # display-plugins 
    # auto-updater
)


target_link_libraries(native-lib android log m)
target_opengl()
target_bullet()

