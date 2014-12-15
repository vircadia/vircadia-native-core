set(TARGET_NAME render-utils)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Widgets OpenGL Network Script)

include_glm()

link_hifi_libraries(shared gpu)
if (APPLE)
  # link in required OS X frameworks and include the right GL headers
  find_library(OpenGL OpenGL)

  target_link_libraries(${TARGET_NAME} ${OpenGL})
    
else (APPLE)
  find_package(OpenGL REQUIRED)
  
  if (${OPENGL_INCLUDE_DIR})
    include_directories(SYSTEM "${OPENGL_INCLUDE_DIR}")
  endif ()
  
  #target_link_libraries(${TARGET_NAME} "${OPENGL_LIBRARY}")
  
  # link target to external libraries
  if (WIN32)
    find_package(GLEW REQUIRED)
    include_directories(${GLEW_INCLUDE_DIRS})

    # we're using static GLEW, so define GLEW_STATIC
    add_definitions(-DGLEW_STATIC)

    #target_link_libraries(${TARGET_NAME} "${GLEW_LIBRARIES}" "${NSIGHT_LIBRARIES}" opengl32.lib)

    # try to find the Nsight package and add it to the build if we find it
    #find_package(NSIGHT)
    #if (NSIGHT_FOUND)
    #  include_directories(${NSIGHT_INCLUDE_DIRS})
    #  add_definitions(-DNSIGHT_FOUND)
    #  #target_link_libraries(${TARGET_NAME} "${NSIGHT_LIBRARIES}")
    #endif ()

  endif()
endif (APPLE)

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies()
