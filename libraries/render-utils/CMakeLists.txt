set(TARGET_NAME render-utils)

AUTOSCRIBE_SHADER_LIB()

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Widgets OpenGL Network Script)

include_glm()

link_hifi_libraries(animation fbx shared gpu)

if (WIN32)
  # we're using static GLEW, so define GLEW_STATIC
  add_definitions(-DGLEW_STATIC)
endif ()

# call macro to include our dependency includes and bubble them up via a property on our target
include_dependency_includes()