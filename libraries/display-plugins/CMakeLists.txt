set(TARGET_NAME display-plugins)

add_definitions(-DGLEW_STATIC)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(OpenGL)

setup_hifi_opengl()

link_hifi_libraries(shared plugins gpu render-utils)

GroupSources("src/display-plugins")

add_dependency_external_projects(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})

add_dependency_external_projects(boostconfig)
find_package(BoostConfig REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${BOOSTCONFIG_INCLUDE_DIRS})

add_dependency_external_projects(oglplus)
find_package(OGLPLUS REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${OGLPLUS_INCLUDE_DIRS})

add_dependency_external_projects(LibOVR)
find_package(LibOVR REQUIRED)
target_include_directories(${TARGET_NAME} PRIVATE ${LIBOVR_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${LIBOVR_LIBRARIES})

if (WIN32)
  add_dependency_external_projects(OpenVR)
  find_package(OpenVR REQUIRED)
  target_include_directories(${TARGET_NAME} PRIVATE ${OPENVR_INCLUDE_DIRS})
  target_link_libraries(${TARGET_NAME} ${OPENVR_LIBRARIES})
endif()
