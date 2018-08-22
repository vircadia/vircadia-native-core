set(TARGET_NAME render)
setup_hifi_library()

# render needs octree only for getAccuracyAngle(float, int)
link_hifi_libraries(shared task ktx gpu shaders graphics octree)

target_nsight()
