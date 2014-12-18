set(TARGET_NAME render-utils)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Widgets OpenGL Network Script)

include_glm()

link_hifi_libraries(shared gpu)

find_package(GLUT REQUIRED)
include_directories(SYSTEM "${GLUT_INCLUDE_DIRS}")

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies()
