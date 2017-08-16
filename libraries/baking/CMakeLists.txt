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
endif ()

set_target_properties(${TARGET_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE EXCLUDE_FROM_DEFAULT_BUILD TRUE)

link_hifi_libraries(shared model networking ktx image)
include_hifi_library_headers(gpu)
