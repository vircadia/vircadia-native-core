set(TARGET_NAME gpu)

AUTOSCRIBE_SHADER_LIB(gpu)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library()

link_hifi_libraries(shared)

if(ANDROID)

  target_link_libraries(${TARGET_NAME} "-lGLESv3" "-lEGL")

else()

  add_dependency_external_projects(glew)
  find_package(GLEW REQUIRED)
  target_include_directories(${TARGET_NAME} PUBLIC ${GLEW_INCLUDE_DIRS})
  
  target_link_libraries(${TARGET_NAME} ${GLEW_LIBRARIES})

  find_package(OpenGL REQUIRED)
  target_link_libraries(${TARGET_NAME} "${OPENGL_LIBRARY}")

endif()

if (WIN32)
  if (USE_NSIGHT)
    # try to find the Nsight package and add it to the build if we find it
    find_package(NSIGHT)
    if (NSIGHT_FOUND)
      include_directories(${NSIGHT_INCLUDE_DIRS})
      add_definitions(-DNSIGHT_FOUND)
      target_link_libraries(${TARGET_NAME} "${NSIGHT_LIBRARIES}")
    endif ()
  endif()
endif ()
