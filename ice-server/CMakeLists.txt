set(TARGET_NAME ice-server)

# setup the project and link required Qt modules
setup_hifi_project(Network)

# link the shared hifi libraries
link_hifi_libraries(embedded-webserver networking shared)

copy_dlls_beside_windows_executable()
