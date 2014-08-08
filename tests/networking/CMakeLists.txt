set(TARGET_NAME networking-tests)

setup_hifi_project(${TARGET_NAME} TRUE)

# link in the shared libraries
link_hifi_libraries(${TARGET_NAME} shared networking)

IF (WIN32)
    target_link_libraries(${TARGET_NAME} Winmm Ws2_32)
ENDIF(WIN32)

