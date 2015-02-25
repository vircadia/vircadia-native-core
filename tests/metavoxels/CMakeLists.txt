set(TARGET_NAME metavoxel-tests)

auto_mtc()

setup_hifi_project(Network Script Widgets)

# link in the shared libraries
link_hifi_libraries(metavoxels networking shared)

copy_dlls_beside_windows_executable()