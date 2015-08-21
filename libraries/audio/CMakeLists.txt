set(TARGET_NAME audio)

setup_memory_debugger()

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Network)

add_dependency_external_projects(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})

# we use libsoxr for resampling
add_dependency_external_projects(soxr)
find_package(Soxr REQUIRED)
target_link_libraries(${TARGET_NAME} ${SOXR_LIBRARIES})
target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${SOXR_INCLUDE_DIRS})

link_hifi_libraries(networking shared)
