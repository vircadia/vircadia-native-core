set(TARGET_NAME networking)
setup_hifi_library(Network WebSockets)
link_hifi_libraries(shared platform)

target_openssl()
target_tbb()

if (WIN32 OR (UNIX AND NOT APPLE))
    target_webrtc()
endif ()

if (WIN32)
    # we need ws2_32.lib on windows, but it's static so we don't bubble it up
    # Libraries needed for WebRTC: security.lib winmm.lib
    target_link_libraries(${TARGET_NAME} ws2_32.lib security.lib winmm.lib)
elseif(APPLE)
    # IOKit is needed for getting machine fingerprint
    find_library(FRAMEWORK_IOKIT IOKit)
    find_library(CORE_FOUNDATION CoreFoundation)
    target_link_libraries(${TARGET_NAME} ${FRAMEWORK_IOKIT} ${CORE_FOUNDATION})
elseif(UNIX)
    # libcrypto uses dlopen in libdl
    target_link_libraries(${TARGET_NAME} ${CMAKE_DL_LIBS})
endif ()
