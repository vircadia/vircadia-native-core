set(TARGET_NAME physics)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library()

include_glm()
include_bullet()

link_hifi_libraries(shared fbx entities)
include_hifi_library_headers(fbx)

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies()
