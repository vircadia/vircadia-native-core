rem Copy this file to a directory above the WebRTC \src directory and run it from there in a command window.
set WEBRTC_SRC_DIR=src
set RELEASE_LIB_DIR=%WEBRTC_SRC_DIR%\out\Release\obj
set DEBUG_LIB_DIR=%WEBRTC_SRC_DIR%\out\Debug\obj
set VCPKG_TGT_DIR=vcpkg

if exist %VCPKG_TGT_DIR% rd /s /q %VCPKG_TGT_DIR%
mkdir %VCPKG_TGT_DIR%

rem License and .lib files
mkdir %VCPKG_TGT_DIR%\webrtc\share\webrtc\
copy %WEBRTC_SRC_DIR%\LICENSE %VCPKG_TGT_DIR%\webrtc\share\webrtc\copyright
xcopy /v %RELEASE_LIB_DIR%\webrtc.lib %VCPKG_TGT_DIR%\webrtc\lib\
xcopy /v %DEBUG_LIB_DIR%\webrtc.lib %VCPKG_TGT_DIR%\webrtc\debug\lib\

rem Header files
mkdir %VCPKG_TGT_DIR%\webrtc\include\webrtc\
copy %WEBRTC_SRC_DIR%\common_types.h %VCPKG_TGT_DIR%\webrtc\include\webrtc
xcopy /v /s /i %WEBRTC_SRC_DIR%\api\*.h %VCPKG_TGT_DIR%\webrtc\include\webrtc\api
xcopy /v /s /i %WEBRTC_SRC_DIR%\audio\*.h %VCPKG_TGT_DIR%\webrtc\include\webrtc\audio
xcopy /v /s /i %WEBRTC_SRC_DIR%\base\*.h %VCPKG_TGT_DIR%\webrtc\include\webrtc\base
xcopy /v /s /i %WEBRTC_SRC_DIR%\call\*.h %VCPKG_TGT_DIR%\webrtc\include\webrtc\call
xcopy /v /s /i %WEBRTC_SRC_DIR%\common_audio\*.h %VCPKG_TGT_DIR%\webrtc\include\webrtc\common_audio
xcopy /v /s /i %WEBRTC_SRC_DIR%\common_video\*.h %VCPKG_TGT_DIR%\webrtc\include\webrtc\common_video
xcopy /v /s /i %WEBRTC_SRC_DIR%\logging\*.h %VCPKG_TGT_DIR%\webrtc\include\webrtc\logging
xcopy /v /s /i %WEBRTC_SRC_DIR%\media\*.h %VCPKG_TGT_DIR%\webrtc\include\webrtc\media
xcopy /v /s /i %WEBRTC_SRC_DIR%\modules\*.h %VCPKG_TGT_DIR%\webrtc\include\webrtc\modules
xcopy /v /s /i %WEBRTC_SRC_DIR%\p2p\*.h %VCPKG_TGT_DIR%\webrtc\include\webrtc\p2p
xcopy /v /s /i %WEBRTC_SRC_DIR%\pc\*.h %VCPKG_TGT_DIR%\webrtc\include\webrtc\pc
xcopy /v /s /i %WEBRTC_SRC_DIR%\rtc_base\*.h %VCPKG_TGT_DIR%\webrtc\include\webrtc\rtc_base
xcopy /v /s /i %WEBRTC_SRC_DIR%\rtc_tools\*.h %VCPKG_TGT_DIR%\webrtc\include\webrtc\rtc_tools
xcopy /v /s /i %WEBRTC_SRC_DIR%\stats\*.h %VCPKG_TGT_DIR%\webrtc\include\webrtc\stats
xcopy /v /s /i %WEBRTC_SRC_DIR%\system_wrappers\*.h %VCPKG_TGT_DIR%\webrtc\include\webrtc\system_wrappers
xcopy /v /s /i %WEBRTC_SRC_DIR%\third_party\abseil-cpp\absl\*.h %VCPKG_TGT_DIR%\webrtc\include\webrtc\absl
xcopy /v /s /i %WEBRTC_SRC_DIR%\third_party\libyuv\include\libyuv\*.h %VCPKG_TGT_DIR%\webrtc\include\webrtc\libyuv
xcopy /v /s /i %WEBRTC_SRC_DIR%\video\*.h %VCPKG_TGT_DIR%\webrtc\include\webrtc\video
