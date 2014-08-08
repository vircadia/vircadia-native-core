set(TARGET_NAME octree-tests)

setup_hifi_project(${TARGET_NAME} TRUE)

# link in the shared libraries
link_hifi_libraries(${TARGET_NAME} octree)

IF (WIN32)
    # add a definition for ssize_t so that windows doesn't bail
    add_definitions(-Dssize_t=long)

    #target_link_libraries(${TARGET_NAME} Winmm Ws2_32)
    target_link_libraries(${TARGET_NAME} wsock32.lib)
ENDIF(WIN32)


