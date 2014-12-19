set(TARGET_NAME gpu)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library()

include_glm()

link_hifi_libraries(shared)

if (APPLE)
  # link in required OS X frameworks and include the right GL headers
  find_library(OpenGL OpenGL)

  target_link_libraries(${TARGET_NAME} ${OpenGL})
elseif (WIN32)
  find_package(GLEW REQUIRED)
  include_directories(${GLEW_INCLUDE_DIRS})
  
  # we're using static GLEW, so define GLEW_STATIC
  add_definitions(-DGLEW_STATIC)

  target_link_libraries(${TARGET_NAME} ${GLEW_LIBRARIES} opengl32.lib)
  
  # need to bubble up the GLEW_INCLUDE_DIRS
  list(APPEND ${TARGET_NAME}_DEPENDENCY_INCLUDES "${GLEW_INCLUDE_DIRS}")

  # try to find the Nsight package and add it to the build if we find it
  find_package(NSIGHT)
  if (NSIGHT_FOUND)
    include_directories(${NSIGHT_INCLUDE_DIRS})
    add_definitions(-DNSIGHT_FOUND)
    target_link_libraries(${TARGET_NAME} "${NSIGHT_LIBRARIES}")
  endif ()
else ()
  find_package(OpenGL REQUIRED)
  
  if (${OPENGL_INCLUDE_DIR})
    include_directories(SYSTEM "${OPENGL_INCLUDE_DIR}")
  endif ()
  
  target_link_libraries(${TARGET_NAME} "${OPENGL_LIBRARY}")
  
  # need to bubble up the OPENGL_INCLUDE_DIR
  list(APPEND ${TARGET_NAME}_DEPENDENCY_INCLUDES "${OPENGL_INCLUDE_DIR}")
endif (APPLE)

# call macro to include our dependency includes and bubble them up via a property on our target
include_dependency_includes()
