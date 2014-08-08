set(TARGET_NAME animation)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(${TARGET_NAME} Network Script)

link_hifi_libraries(${TARGET_NAME} shared fbx)