set(TARGET_NAME shared-tests)

setup_hifi_project()

# link in the shared libraries
link_hifi_libraries(shared)

copy_dlls_beside_windows_executable()