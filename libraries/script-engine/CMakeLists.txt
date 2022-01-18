set(TARGET_NAME script-engine)
# FIXME Move undo scripting interface to application and remove Widgets
setup_hifi_library(Gui Network Script ScriptTools WebSockets Widgets)

target_zlib()
if (NOT ANDROID)
  target_quazip()
endif ()

link_hifi_libraries(shared networking shaders material-networking model-networking recording avatars model-serializers entities controllers animation audio midi)
include_hifi_library_headers(gl)
include_hifi_library_headers(hfm)
include_hifi_library_headers(gpu)
include_hifi_library_headers(ktx)
include_hifi_library_headers(image)
include_hifi_library_headers(graphics)
include_hifi_library_headers(octree)
include_hifi_library_headers(procedural)