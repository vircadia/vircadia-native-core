set(TARGET_NAME script-engine)
# FIXME Move undo scripting interface to application and remove Widgets
setup_hifi_library(Gui Network Script ScriptTools WebSockets Widgets)

target_zlib()

if (NOT ANDROID)

  add_dependency_external_projects(quazip)
  find_package(QuaZip REQUIRED)
  target_include_directories(${TARGET_NAME} SYSTEM PUBLIC ${QUAZIP_INCLUDE_DIRS})
  target_link_libraries(${TARGET_NAME} ${QUAZIP_LIBRARIES})

  if (WIN32)
    add_paths_to_fixup_libs(${QUAZIP_DLL_PATH})
  endif ()

endif ()

link_hifi_libraries(shared networking shaders material-networking model-networking recording avatars fbx entities controllers animation audio midi)
include_hifi_library_headers(gl)
include_hifi_library_headers(hfm)
include_hifi_library_headers(gpu)
include_hifi_library_headers(ktx)
include_hifi_library_headers(image)
include_hifi_library_headers(graphics)
include_hifi_library_headers(octree)
include_hifi_library_headers(procedural)