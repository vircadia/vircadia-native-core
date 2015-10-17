set(TARGET_NAME udt-test)
setup_hifi_project()

link_hifi_libraries(networking shared)

copy_dlls_beside_windows_executable()