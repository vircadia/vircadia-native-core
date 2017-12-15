set(TARGET_NAME oven)

setup_hifi_project(Widgets Gui Concurrent)

link_hifi_libraries(networking shared image gpu ktx fbx baking model)

setup_memory_debugger()

if (WIN32)
  package_libraries_for_deployment()
elseif (UNIX AND NOT APPLE)
  find_package(Threads REQUIRED)
  if(THREADS_HAVE_PTHREAD_ARG)
    target_compile_options(PUBLIC oven "-pthread")
  endif()
elseif (APPLE)
  # Fix up the rpath so macdeployqt works
  set_target_properties(${TARGET_NAME} PROPERTIES INSTALL_RPATH "@executable_path/../Frameworks")
endif()

install_beside_console()
