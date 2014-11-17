set(TARGET_NAME gvr-interface)

set(${TARGET_NAME}_SRCS
  ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

set(${TARGET_NAME}_SHARED true)
setup_hifi_library()

include_directories(${ANDROID_NDK}/sources/android/native_app_glue)

include_glm()

set(REQUIRED_HIFI_LIBRARIES "shared" "networking")

link_hifi_libraries(${REQUIRED_HIFI_LIBRARIES})
link_shared_dependencies()

target_link_libraries(${TARGET_NAME} log android)
set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_DEFINITIONS "ANDROID")

set(ANDROID_API_LEVEL 19)
set(ARM_TARGET "armeabi-v7a")

foreach(TARGET_LINK_LIBRARY ${${TARGET_NAME}_LIBRARIES_TO_LINK}) 
  get_filename_component(LIB_EXTENSION ${TARGET_LINK_LIBRARY} NAME_WE)
  
  if (LIB_EXTENSION STREQUAL "so")
    list(APPEND TARGET_SHARED_LIBRARIES ${TARGET_LINK_LIBRARY})
  else ()
    list(APPEND TARGET_STATIC_LIBRARIES ${TARGET_LINK_LIBRARY})
  endif ()
  
endforeach()

# append each of the hifi shared libraries to our list of libs to link
foreach(HIFI_STATIC_LIBRARY ${REQUIRED_HIFI_LIBRARIES})
  list(APPEND TARGET_STATIC_LIBRARIES "${LIBRARY_OUTPUT_PATH}/lib${HIFI_STATIC_LIBRARY}.a")
endforeach()

list(APPEND TARGET_SHARED_LIBRARIES "${LIBRARY_OUTPUT_PATH}/lib${TARGET_NAME}.so")

set(TARGET_JAR_LIBRARIES "${QT_CMAKE_PREFIX_PATH}/../../jar/QtAndroid-bundled.jar")

android_create_apk(
  ${TARGET_NAME}
  "${CMAKE_BINARY_DIR}/apk"
  "${TARGET_SHARED_LIBRARIES}"
  "${TARGET_STATIC_LIBRARIES}"
  "${TARGET_JAR_LIBRARIES}"
  "${CMAKE_CURRENT_SOURCE_DIR}/assets"
  "Data"
)