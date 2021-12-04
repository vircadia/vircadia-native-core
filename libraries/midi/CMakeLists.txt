set(TARGET_NAME midi)
setup_hifi_library(Network)
link_hifi_libraries(shared networking)

if (WIN32)
    target_link_libraries(${TARGET_NAME} winmm.lib)
endif ()
