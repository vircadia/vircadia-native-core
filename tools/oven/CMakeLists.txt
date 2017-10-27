set(TARGET_NAME oven)

setup_hifi_project(Widgets Gui Concurrent)

link_hifi_libraries(networking shared image gpu ktx fbx baking model)

setup_memory_debugger()

if (WIN32)
  package_libraries_for_deployment()
endif ()

if (UNIX)
  find_package(Threads REQUIRED)
  if(THREADS_HAVE_PTHREAD_ARG)
    target_compile_options(PUBLIC oven "-pthread")
  endif()
endif ()

set_target_properties(${TARGET_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE EXCLUDE_FROM_DEFAULT_BUILD TRUE)
