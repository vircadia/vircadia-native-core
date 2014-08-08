set(TARGET_NAME networking)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(${TARGET_NAME} Network)

link_hifi_libraries(${TARGET_NAME} shared)