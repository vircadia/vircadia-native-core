set(TARGET_NAME script-engine)
setup_hifi_library(Gui Network Script WebSockets Widgets)
link_hifi_libraries(shared networking octree gpu ui procedural model model-networking recording avatars fbx entities controllers animation audio physics)
