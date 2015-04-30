set(TARGET_NAME render-utils)

AUTOSCRIBE_SHADER_LIB(gpu model)

# pull in the resources.qrc file
qt5_add_resources(QT_RESOURCES_FILE "${CMAKE_CURRENT_SOURCE_DIR}/res/fonts/fonts.qrc")

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Widgets OpenGL Network Qml Quick Script)

add_dependency_external_projects(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})

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

link_hifi_libraries(animation fbx shared gpu)