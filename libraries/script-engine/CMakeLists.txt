set(TARGET_NAME script-engine)
setup_hifi_library(Gui Network Script ScriptTools WebSockets Widgets)

target_zlib()

add_dependency_external_projects(quazip)
find_package(QuaZip REQUIRED)
target_include_directories(${TARGET_NAME} SYSTEM PUBLIC ${QUAZIP_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${QUAZIP_LIBRARIES})

link_hifi_libraries(shared networking octree gpu ui procedural model model-networking recording avatars fbx entities controllers animation audio physics)
