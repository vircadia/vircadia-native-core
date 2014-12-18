set(TARGET_NAME render-utils)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Widgets OpenGL Network Script)

include_glm()

link_hifi_libraries(animation fbx shared gpu)

if (APPLE)
  find_library(GLUT GLUT)
  target_link_libraries(${TARGET_NAME} ${GLUT})
else ()
  find_package(GLUT REQUIRED)
  include_directories(SYSTEM "${GLUT_INCLUDE_DIRS}")
  target_link_libraries(${TARGET_NAME} ${GLUT_LIBRARIES})
endif ()

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies()
