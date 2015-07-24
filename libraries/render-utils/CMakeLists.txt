set(TARGET_NAME render-utils)

AUTOSCRIBE_SHADER_LIB(gpu model render)

# pull in the resources.qrc file
qt5_add_resources(QT_RESOURCES_FILE "${CMAKE_CURRENT_SOURCE_DIR}/res/fonts/fonts.qrc")

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Widgets OpenGL Network Qml Quick Script)

setup_hifi_opengl()

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

  add_dependency_external_projects(boostconfig)
  find_package(BoostConfig REQUIRED)
  target_include_directories(${TARGET_NAME} PUBLIC ${BOOSTCONFIG_INCLUDE_DIRS})

  add_dependency_external_projects(oglplus)
  find_package(OGLPLUS REQUIRED)
  target_include_directories(${TARGET_NAME} PUBLIC ${OGLPLUS_INCLUDE_DIRS})
endif (WIN32)

link_hifi_libraries(animation fbx shared gpu model render environment)
