set(TARGET_NAME octree-tests)

setup_hifi_project(Script Network)

# link in the shared libraries
link_hifi_libraries(shared octree gpu model fbx networking environment entities avatars audio animation script-engine physics)

copy_dlls_beside_windows_executable()