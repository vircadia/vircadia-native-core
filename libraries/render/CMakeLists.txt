set(TARGET_NAME render)
AUTOSCRIBE_SHADER_LIB(gpu model)
setup_hifi_library()

link_hifi_libraries(shared ktx gpu model)
# render needs octree only for getAccuracyAngle(float, int)
include_hifi_library_headers(octree)

target_nsight()
