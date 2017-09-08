set(TARGET_NAME baking)
setup_hifi_library(Concurrent)

link_hifi_libraries(shared model networking ktx image fbx)
include_hifi_library_headers(gpu)

add_dependency_external_projects(draco)
find_package(Draco REQUIRED)
target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${DRACO_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${DRACO_LIBRARY} ${DRACO_ENCODER_LIBRARY})
