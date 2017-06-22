set(TARGET_NAME oven)

setup_hifi_project(Widgets Gui Concurrent)

link_hifi_libraries(networking shared image gpu ktx)

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

# try to find the FBX SDK but fail silently if we don't
# because this tool is not built by default
find_package(FBX)
if (FBX_FOUND)
  if (CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(${TARGET_NAME} ${FBX_LIBRARIES} "${CMAKE_THREAD_LIBS_INIT}")
  else ()
    target_link_libraries(${TARGET_NAME} ${FBX_LIBRARIES})
  endif ()
  target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${FBX_INCLUDE_DIR})
endif ()

set_target_properties(${TARGET_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE EXCLUDE_FROM_DEFAULT_BUILD TRUE)
