set(TARGET_NAME model-baking)

setup_hifi_library()

find_package(FBXSDK REQUIRED)
target_link_libraries(${TARGET_NAME} ${FBX_LIBRARIES})
target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${FBX_INCLUDE_DIR})
