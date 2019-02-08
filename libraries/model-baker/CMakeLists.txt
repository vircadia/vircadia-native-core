set(TARGET_NAME model-baker)
setup_hifi_library()

link_hifi_libraries(shared shaders task gpu graphics hfm material-networking)
include_hifi_library_headers(networking)