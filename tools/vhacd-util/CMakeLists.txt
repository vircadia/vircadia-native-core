set(TARGET_NAME vhacd-util)
setup_hifi_project(Core)
link_hifi_libraries(shared model-serializers hfm graphics gpu gl)

include_hifi_library_headers(image)

target_vhacd()

setup_memory_debugger()
setup_thread_debugger()

if (WIN32)
  package_libraries_for_deployment()
elseif (UNIX AND NOT APPLE)
  include(FindOpenMP)
  if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
  endif()
endif ()

install_beside_console()
