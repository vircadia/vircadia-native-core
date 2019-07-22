include(vcpkg_common_functions)
set(WEBRTC_VERSION 20190626)
set(MASTER_COPY_SOURCE_PATH ${CURRENT_BUILDTREES_DIR}/src)

if (ANDROID)
   # this is handled by hifi_android.py
elseif (WIN32)
    vcpkg_download_distfile(
        WEBRTC_SOURCE_ARCHIVE
        URLS https://hifi-public.s3.amazonaws.com/seth/webrtc-20190626-windows.zip
        SHA512 c0848eddb1579b3bb0496b8785e24f30470f3c477145035fd729264a326a467b9467ae9f426aa5d72d168ad9e9bf2c279150744832736bdf39064d24b04de1a3
        FILENAME webrtc-20190626-windows.zip
    )
elseif (APPLE)
    vcpkg_download_distfile(
        WEBRTC_SOURCE_ARCHIVE
        URLS https://hifi-public.s3.amazonaws.com/seth/webrtc-20190626-osx.tar.gz
        SHA512 fc70cec1b5ee87395137b7090f424e2fc2300fc17d744d5ffa1cf7aa0e0f1a069a9d72ba1ad2fb4a640ebeb6c218bda24351ba0083e1ff96c4a4b5032648a9d2
        FILENAME webrtc-20190626-osx.tar.gz
    )
else ()
    # else Linux desktop
    vcpkg_download_distfile(
        WEBRTC_SOURCE_ARCHIVE
        URLS https://hifi-public.s3.amazonaws.com/seth/webrtc-20190626-linux.tar.gz
        SHA512 07d7776551aa78cb09a3ef088a8dee7762735c168c243053b262083d90a1d258cec66dc386f6903da5c4461921a3c2db157a1ee106a2b47e7756cb424b66cc43
        FILENAME webrtc-20190626-linux.tar.gz
    )
endif ()

vcpkg_extract_source_archive(${WEBRTC_SOURCE_ARCHIVE})

file(COPY ${MASTER_COPY_SOURCE_PATH}/webrtc/include DESTINATION ${CURRENT_PACKAGES_DIR})
file(COPY ${MASTER_COPY_SOURCE_PATH}/webrtc/lib DESTINATION ${CURRENT_PACKAGES_DIR})
file(COPY ${MASTER_COPY_SOURCE_PATH}/webrtc/share DESTINATION ${CURRENT_PACKAGES_DIR})
file(COPY ${MASTER_COPY_SOURCE_PATH}/webrtc/debug DESTINATION ${CURRENT_PACKAGES_DIR})
