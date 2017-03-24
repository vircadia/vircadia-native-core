set(TARGET_NAME render)
AUTOSCRIBE_SHADER_LIB(gpu model)
setup_hifi_library()

# render needs octree only for getAccuracyAngle(float, int)
link_hifi_libraries(shared ktx gpu model octree)

target_nsight()
