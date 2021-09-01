set(TARGET_NAME animation)
setup_hifi_library(Network)
link_hifi_libraries(shared graphics model-serializers script-engine)
include_hifi_library_headers(gpu)
include_hifi_library_headers(hfm)
include_hifi_library_headers(image)
include_hifi_library_headers(networking)

target_nsight()

if (WIN32)
  add_compile_definitions(_USE_MATH_DEFINES)
endif()
