set(TARGET_NAME script-engine-qtscript)
# FIXME Move undo scripting interface to application and remove Widgets
setup_hifi_library(Gui Script ScriptTools)

link_hifi_libraries()
include_hifi_library_headers(animation)
include_hifi_library_headers(entities)
include_hifi_library_headers(networking)
include_hifi_library_headers(shared)
include_hifi_library_headers(script-engine)