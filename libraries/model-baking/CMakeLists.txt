set(TARGET_NAME model-baking)

setup_hifi_library(Concurrent)

link_hifi_libraries(networking image gpu shared ktx)

# try to find the FBX SDK but fail silently if we don't
# because this library is not built by default
find_package(FBX)
if (FBX_FOUND)
  target_link_libraries(${TARGET_NAME} ${FBX_LIBRARIES})
  target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${FBX_INCLUDE_DIR})
endif ()

set_target_properties(${TARGET_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE EXCLUDE_FROM_DEFAULT_BUILD TRUE)
