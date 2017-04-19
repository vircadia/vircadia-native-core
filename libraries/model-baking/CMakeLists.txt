set(TARGET_NAME model-baking)

setup_hifi_library(Concurrent)

link_hifi_libraries(networking image gpu shared ktx)

find_package(FBXSDK)
target_link_libraries(${TARGET_NAME} ${FBX_LIBRARIES})
target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${FBX_INCLUDE_DIR})

set_target_properties(${TARGET_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE EXCLUDE_FROM_DEFAULT_BUILD TRUE)
