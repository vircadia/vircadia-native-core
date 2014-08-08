set(TARGET_NAME octree-tests)

setup_hifi_project(${TARGET_NAME} TRUE)

# link in the shared libraries
link_hifi_libraries(${TARGET_NAME} octree)

IF (WIN32)
  target_link_libraries(${TARGET_NAME} wsock32.lib)
ENDIF(WIN32)
