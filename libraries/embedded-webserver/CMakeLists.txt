set(TARGET_NAME embedded-webserver)

setup_memory_debugger()

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Network)
