project(gvr-builder)

include_directories(${ANDROID_NDK}/sources/android/native_app_glue)

set(GVR_SRC
  ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
  src/main.cpp
)

add_library(gvr-interface SHARED ${GVR_SRC})

target_link_libraries(gvr-interface log android)
set_target_properties(gvr-interface PROPERTIES COMPILE_DEFINITIONS "ANDROID")
android_create_apk(gvr-interface "io.highfidelity.interface"
  "${CMAKE_BINARY_DIR}/apk" 
  "${CMAKE_BINARY_DIR}/libs" 
  "${CMAKE_CURRENT_SOURCE_DIR}/android" 
  "${CMAKE_CURRENT_SOURCE_DIR}/assets"
)