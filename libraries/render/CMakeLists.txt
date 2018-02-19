set(TARGET_NAME render)
AUTOSCRIBE_SHADER_LIB(gpu graphics)
setup_hifi_library()

# render needs octree only for getAccuracyAngle(float, int)
link_hifi_libraries(shared task ktx gpu graphics octree)

target_nsight()
