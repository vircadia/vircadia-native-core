set(TARGET_NAME voxel-edit)

setup_hifi_project(${TARGET_NAME} TRUE)

# add a definition for ssize_t so that windows doesn't bail
if (WIN32)
  add_definitions(-Dssize_t=long)
endif ()