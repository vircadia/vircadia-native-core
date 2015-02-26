set(TARGET_NAME vhacd)
setup_hifi_project()
link_hifi_libraries(shared model fbx gpu networking octree)

#find_package(VHACD REQUIRED) done in CMakeList.txt in parent directory
target_include_directories(${TARGET_NAME} PUBLIC ${VHACD_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${VHACD_LIBRARIES})

if(NOT WIN32)
    find_package( Threads)
    target_link_libraries(${TARGET_NAME} ${CMAKE_THREAD_LIBS_INIT})

    include(FindOpenMP)
    if(OPENMP_FOUND)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
   endif()
endif()

add_dependency_external_project(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})

include_dependency_includes()
