set(TARGET_NAME avatars-renderer)
AUTOSCRIBE_SHADER_LIB(gpu model render render-utils)
setup_hifi_library(Widgets Network Script)
link_hifi_libraries(shared gpu model animation physics model-networking script-engine render image render-utils)

target_bullet()
