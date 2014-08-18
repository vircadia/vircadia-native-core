set(TARGET_NAME octree-tests)

setup_hifi_project(Script Network)

include_glm()

# link in the shared libraries
//link_hifi_library(entities ${TARGET_NAME} ${ROOT_DIR})
//link_hifi_library(octree ${TARGET_NAME} ${ROOT_DIR})
//link_hifi_library(audio ${TARGET_NAME} ${ROOT_DIR})
//link_hifi_library(networking ${TARGET_NAME} ${ROOT_DIR})
//link_hifi_library(animation ${TARGET_NAME} ${ROOT_DIR})
//link_hifi_library(fbx ${TARGET_NAME} ${ROOT_DIR})
//link_hifi_library(shared ${TARGET_NAME} ${ROOT_DIR})
//link_hifi_library(voxels ${TARGET_NAME} ${ROOT_DIR})

link_hifi_libraries(animation fbx entities networking octree shared)

link_shared_dependencies()
