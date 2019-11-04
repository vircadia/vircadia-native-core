set(TARGET_NAME model-baker)
setup_hifi_library()

link_hifi_libraries(shared shaders task gpu graphics hfm procedural)
include_hifi_library_headers(material-networking)
include_hifi_library_headers(networking)
include_hifi_library_headers(image)
include_hifi_library_headers(ktx)

target_draco()
