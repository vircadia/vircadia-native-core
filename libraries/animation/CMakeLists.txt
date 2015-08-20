set(TARGET_NAME animation)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Network Script)

setup_memory_debugger()

link_hifi_libraries(shared gpu model fbx)
