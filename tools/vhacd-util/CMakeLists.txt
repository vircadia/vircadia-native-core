set(TARGET_NAME vhacd-util)
setup_hifi_project(Core Widgets)
link_hifi_libraries(shared fbx model gpu gl)

add_dependency_external_projects(vhacd)

find_package(VHACD REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${VHACD_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${VHACD_LIBRARIES})

if (UNIX AND NOT APPLE)
  include(FindOpenMP)
  if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
  endif()
endif ()
