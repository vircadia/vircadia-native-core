set(TARGET_NAME render)

AUTOSCRIBE_SHADER_LIB(gpu model)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library()

add_dependency_external_projects(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})

link_hifi_libraries(shared gpu model)

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
endif (WIN32)