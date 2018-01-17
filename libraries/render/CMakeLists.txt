set(TARGET_NAME render)
AUTOSCRIBE_SHADER_LIB(gpu graphics)
setup_hifi_library()

# render needs octree only for getAccuracyAngle(float, int), and model-networking for TextureCache
link_hifi_libraries(shared ktx gpu graphics octree model-networking)
include_hifi_library_headers(networking)
include_hifi_library_headers(image)

target_nsight()
