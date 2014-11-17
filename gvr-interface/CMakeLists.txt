set(TARGET_NAME gvr-interface)

set(BUILD_SHARED_LIBS ON)

set(${TARGET_NAME}_SRCS
  ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

setup_hifi_library()

include_directories(${ANDROID_NDK}/sources/android/native_app_glue)

# include_glm()
#
# link_hifi_libraries(shared networking)
link_shared_dependencies()

target_link_libraries(${TARGET_NAME} log android)
set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_DEFINITIONS "ANDROID")

set(ANDROID_API_LEVEL 19)
set(ARM_TARGET "armeabi-v7a")

set(TARGET_SHARED_LIBRARIES ${${TARGET_NAME}_LIBRARIES_TO_LINK})
list(APPEND TARGET_SHARED_LIBRARIES "${LIBRARY_OUTPUT_PATH}/lib${TARGET_NAME}.so")

android_create_apk(
  ${TARGET_NAME}
  "${CMAKE_BINARY_DIR}/apk"
  "${TARGET_SHARED_LIBRARIES}"
  "${CMAKE_CURRENT_SOURCE_DIR}/assets"
  "Data"
)