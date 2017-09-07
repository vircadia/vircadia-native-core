set(TARGET_NAME baking)
setup_hifi_library(Concurrent)

find_package(FBX)
if (FBX_FOUND)
  if (CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(${TARGET_NAME} ${FBX_LIBRARIES} "${CMAKE_THREAD_LIBS_INIT}")
  else ()
    target_link_libraries(${TARGET_NAME} ${FBX_LIBRARIES})
  endif ()
  target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${FBX_INCLUDE_DIR})

  if (UNIX)
    target_link_libraries(${TARGET_NAME} ${CMAKE_DL_LIBS})
  endif (UNIX)
endif ()

link_hifi_libraries(shared model networking ktx image fbx)
include_hifi_library_headers(gpu)
