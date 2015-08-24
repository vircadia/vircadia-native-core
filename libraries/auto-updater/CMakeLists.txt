set(TARGET_NAME auto-updater)

setup_memory_debugger()

setup_hifi_library(Network)
link_hifi_libraries(shared networking)
