set(TARGET_NAME gpu)

AUTOSCRIBE_SHADER_LIB(gpu)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library()

link_hifi_libraries(shared)

add_dependency_external_projects(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})

add_dependency_external_projects(glew)
find_package(GLEW REQUIRED)
add_definitions(-DGLEW_STATIC) 
target_include_directories(${TARGET_NAME} PUBLIC ${GLEW_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${GLEW_LIBRARY})

if (APPLE)
  # link in required OS X frameworks and include the right GL headers
  find_library(OpenGL OpenGL)
  target_link_libraries(${TARGET_NAME} ${OpenGL})
elseif (WIN32)
  target_link_libraries(${TARGET_NAME} ${GLEW_LIBRARY} opengl32.lib)

  if (USE_NSIGHT)
    # try to find the Nsight package and add it to the build if we find it
    # note that this will also enable NSIGHT profilers in all the projects linking gpu
    find_package(NSIGHT)
    if (NSIGHT_FOUND)
      target_include_directories(${TARGET_NAME} PUBLIC ${NSIGHT_INCLUDE_DIRS})
      target_compile_definitions(${TARGET_NAME} PUBLIC NSIGHT_FOUND)
      target_link_libraries(${TARGET_NAME} "${NSIGHT_LIBRARIES}")
    endif ()
  endif()
elseif (ANDROID)
  target_link_libraries(${TARGET_NAME} "-lGLESv3" "-lEGL")
else ()

  find_package(OpenGL REQUIRED)
  
  if (${OPENGL_INCLUDE_DIR})
    include_directories(SYSTEM "${OPENGL_INCLUDE_DIR}")
  endif ()

  target_link_libraries(${TARGET_NAME} "${OPENGL_LIBRARY}")
endif (APPLE)
