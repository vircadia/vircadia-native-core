set(TARGET_NAME render)
AUTOSCRIBE_SHADER_LIB(gpu graphics)
setup_hifi_library()

# render needs octree only for getAccuracyAngle(float, int)
link_hifi_libraries(shared ktx gpu graphics octree)
include_hifi_library_headers(task)

target_nsight()
