set(TARGET_NAME render-utils)
AUTOSCRIBE_SHADER_LIB(gpu graphics render)
# pull in the resources.qrc file
qt5_add_resources(QT_RESOURCES_FILE "${CMAKE_CURRENT_SOURCE_DIR}/res/fonts/fonts.qrc")
setup_hifi_library(Gui Network Qml Quick Script)
link_hifi_libraries(shared ktx gpu graphics model-networking render animation fbx image procedural)
include_hifi_library_headers(networking)
include_hifi_library_headers(octree)
include_hifi_library_headers(audio)

if (NOT ANDROID)
  target_nsight()
endif ()
