set(TARGET_NAME physics)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library()

add_dependency_external_projects(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})

add_dependency_external_projects(bullet)

find_package(Bullet REQUIRED)

# perform the system include hack for OS X to ignore warnings
if (APPLE)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${BULLET_INCLUDE_DIRS}")
else()
  target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${BULLET_INCLUDE_DIRS})
endif()

target_link_libraries(${TARGET_NAME} ${BULLET_LIBRARIES})

link_hifi_libraries(shared fbx entities)
include_hifi_library_headers(fbx)
