set(TARGET_NAME entities-renderer)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Widgets OpenGL Network Script)

add_dependency_external_projects(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})

add_dependency_external_projects(bullet)
find_package(Bullet REQUIRED)
target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${BULLET_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${BULLET_LIBRARIES})

add_dependency_external_projects(polyvox)
find_package(PolyVox REQUIRED)
target_include_directories(${TARGET_NAME} SYSTEM PUBLIC ${POLYVOX_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${POLYVOX_LIBRARIES})

# for changing the pitch of collision sounds
add_dependency_external_projects(soxr)
find_package(Soxr REQUIRED)
target_link_libraries(${TARGET_NAME} ${SOXR_LIBRARIES})
target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${SOXR_INCLUDE_DIRS})

link_hifi_libraries(shared gpu script-engine render-utils)
