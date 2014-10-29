set(TARGET_NAME physics)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library()

include_glm()

link_hifi_libraries(shared)

# Note: we rely on the default FindBullet.cmake moduld that comes with cmake
find_package(Bullet)
if (BULLET_FOUND)
    include_directories(SYSTEM "${BULLET_INCLUDE_DIRS}")
    list(APPEND ${TARGET_NAME}_LIBRARIES_TO_LINK "${BULLET_LIBRARIES}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_BULLET_PHYSICS")
endif (BULLET_FOUND)

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies()
