set(TARGET_NAME script-engine)
setup_hifi_library(Gui Network Script WebSockets Widgets)
link_hifi_libraries(shared networking octree gpu procedural model model-networking fbx entities animation audio physics)
