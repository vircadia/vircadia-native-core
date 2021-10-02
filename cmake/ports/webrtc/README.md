# WebRTC

WebRTC Information:
- https://webrtc.org/
- https://webrtc.googlesource.com/src
- https://webrtc.googlesource.com/src/+/refs/heads/master/docs/native-code/index.md
  - https://webrtc.googlesource.com/src/+/refs/heads/master/docs/native-code/development/prerequisite-sw/index.md
  - https://webrtc.googlesource.com/src/+/refs/heads/master/docs/native-code/development/index.md
- https://www.chromium.org/developers/calendar
- https://github.com/microsoft/winrtc
- https://docs.microsoft.com/en-us/winrtc/getting-started
- https://groups.google.com/g/discuss-webrtc \
  See "PSA" posts for release information.
- https://bugs.chromium.org/p/webrtc/issues/list
- https://stackoverflow.com/questions/27809193/webrtc-not-building-for-windows
- https://github.com/aisouard/libwebrtc/issues/57

## Windows - M84

WebRTC's M84 release is currently used because it corresponded to Microsoft's latest WinRTC release at the time of development,
and WinRTC is a source of potentially useful patches.

The following notes document how the M84-based Windows VCPKG was created, using Visual Studio 2019.

### Set Up depot_tools

Install Google's depot_tools.
- Download depot_tools.zip.
- Extract somewhere.
- Add the extracted directory to the start of the system `PATH` environment variable.

Configure depot_tools.
- Set an environment variable `DEPOT_TOOLS_WIN_TOOLCHAIN=0`
- Set an environment variable `GYP_MSVS_VERSION=2019`

Initialize depot_tools.
- VS2019 developer command prompt in the directory where the source tree will be created.  
- `gclient`

### Get the Code

Fetch the code into a *\src* subdirectory. This may take some time!
- `fetch --nohooks webrtc`

Switch to the M84 branch.
- `cd src`
- `git checkout branch-heads/4147`

Fetch all the subrepositories.  
- `gclient sync -D -r branch-heads/4147`

### Patch the Code

#### Modify compiler switches
- Edit *build\config\win\BUILD.gn*:
  - Change all `/MT` to `/MD`, and `/MTd` to `/MDd`.
  - Change all `cflags = [ "/MDd" ]` to `[ "/MDd", "-D_ITERATOR_DEBUG_LEVEL=2", "-D_HAS_ITERATOR_DEBUGGING=1" ]`.
- Edit *build\config\compiler\BUILD.gn*:\
  Change:
  ```
    if (is_win) {
      if (is_clang) {
        cflags = [ "/Z7" ]  # Debug information in the .obj files.
      } else {
        cflags = [ "/Zi" ]  # Produce PDB file, no edit and continue.
      }
  ```
  to:
  ```
    if (is_win) {
      if (is_clang) {
        cflags = [ "/Z7", "/std:c++17", "/Zc:__cplusplus" ]  # Debug information in the .obj files.
      } else {
        cflags = [ "/Zi", "/std:c++17", "/Zc:__cplusplus" ]  # Produce PDB file, no edit and continue.
      }
  ```

#### H265 Codec Fixes  
https://bugs.webrtc.org/9213#c13
- Edit the following files:
  - *modules\video_coding\codecs\h264\h264_color_space.h*
  - *modules\video_coding\codecs\h264\h264_decoder_impl.h*
  - *modules\video_coding\codecs\h264\h264_encoder_impl.h*
  In each, comment out the following lines:
  ```
  #if defined(WEBRTC_WIN) && !defined(__clang__)
  #error "See: bugs.webrtc.org/9213#c13."
  #endif
  ```
- Edit *third_party\ffmpeg\libavcodec\fft_template.c*:\
  Comment out all of `ff_fft_init` except the fail clause at the end.
- Edit *third_party\ffmpeg\libavcodec\pcm.c*:\
  Comment out last line, containing `PCM Archimedes VIDC`.
- Edit *third_party\ffmpeg\libavutil\x86\imgutils_init.c*:\
  Add the following method to the end of the file:
  ```
  void avpriv_emms_asm(void) {}  // Fix missing symbol in FFMPEG.
  ```

#### Exclude BoringSSL
A separate OpenSSL VCPKG is used for building Vircadia.
The following patches are needed even though SSL is excluded in the `gn gen` build commands.
- Rename *third_party\boringssl* to *third_party\boringssl-NO*.
- Edit *third_party\libsrtp\BUILD.gn:\
  Change:
  ```
  public_deps = [
    "//third_party/boringssl:boringssl",
  ]
  ```
  To:
  ```
  public_deps = [
    # "//third_party/boringssl:boringssl",
  ]
  ```

- Edit *third_party\usrsctp\BUILD.gn*:\
  Change:
  ```
  deps = [ "//third_party/boringssl" ]
  ```
  To:
  ```
  deps = [
    # "//third_party/boringssl"
  ]
  ```
- Edit *base\BUILD.gn*:\
  In the code under:
  ```
  # Use the base implementation of hash functions when building for
  # NaCl. Otherwise, use boringssl.
  ```
  Change:
  ```
  if (is_nacl) {
  ```
  To:
  ```
  # if (is_nacl) {
  if (true) {
  ```
- Edit *rtc_base\BUILD.gn*:\
  Change:  
  ```
  if (rtc_build_ssl) {
    deps += [ "//third_party/boringssl" ]
  } else {
  ```
  To:  
  ```
  if (rtc_build_ssl) {
    # deps += [ "//third_party/boringssl" ]
  } else {
  ```

### Set Up OpenSSL

Do one of the following to provide OpenSSL for building against:
a. If you have built Vircadia, find the **HIFI_VCPKG_BASE** subdirectory used in your build and make note of the path to and 
including the *installed\x64-windows\include* directory (which includes an *openssl* directory).
a. Follow https://github.com/vircadia/vcpkg to install *vcpkg* and then *openssl*. Make note of the path to and including the 
*packages\openssl-windows_x64-windows\include* directory (which includes an *openssl* directory).

Copy the *\<path\>\openssl* directory to the following locations (i.e., add as *openssl* subdirectories):
- *third_party\libsrtp\crypto\include*
- *third_party\usrsctp\usrsctplib\usrsctplib*

Also use the path in the `gn gen` commands, below, making sure to escape `\`s as `\\`s.

### Build and Package

Use a VS2019 developer command prompt in the *src* directory.

Create a release build of the WebRTC library:
- `gn gen --ide=vs2019 out\Release --filters=//:webrtc "--args=is_debug=false is_clang=false use_custom_libcxx=false libcxx_is_shared=true symbol_level=2 use_lld=false rtc_include_tests=false rtc_build_tools=false rtc_build_examples=false proprietary_codecs=true rtc_use_h264=true enable_libaom=false rtc_enable_protobuf=false rtc_build_ssl=false rtc_ssl_root=\"<path>\""`
- `ninja -C out\Release`

Create a debug build of the WebRTC library:
- `gn gen --ide=vs2019 out\Debug --filters=//:webrtc "--args=is_debug=true is_clang=false use_custom_libcxx=false libcxx_is_shared=true enable_iterator_debugging=true use_lld=false rtc_include_tests=false rtc_build_tools=false rtc_build_examples=false proprietary_codecs=true rtc_use_h264=true enable_libaom=false rtc_enable_protobuf=false rtc_build_ssl=false rtc_ssl_root=\"<path>\""`
- `ninja -C out\Debug`

Create VCPKG file:
- Assemble files in VCPKG directory structure: Use the *copy-VCPKG-files-win.cmd* batch file per instructions in it.  
`cd ..`\
`copy-VCPKG-files-win`
- Zip up the VCPKG *webrtc* directory created by the batch file.  
`cd vcpkg`\
`7z a -tzip webrtc-m84-yyyymmdd-windows.zip webrtc`
- Calculate the SHA512 of the zip file. E.g., using a Windows PowerShell command window:\
  `Get-FileHash <filename> -Algorithm SHA512 | Format-List`
- Convert the SHA512 to lower case. E.g., using Microsoft Word, select the SHA512 text and use Shift-F3 to change the case.
- Host the zip file on the Web.
- Update *CONTROL* and *portfile.cmake* with version number (revision date), zip file Web URL, and SHA512.

### Tidying up

Disable the depot_tools:
- Rename the *depot_tools* directory so that these tools don't interfere with the rest of your development environment for 
  other work.


## Linux - M81

The original, High Fidelity-provided WebRTC VCPKG library is used for AEC (audio echo cancellation) only.

**TODO:** Update to M84 and include WebRTC components per Windows WebRTC.


## MacOS - M78

The original, High Fidelity-provided WebRTC VCPKG library is used for AEC (audio echo cancellation) only.

**TODO:** Update to M84 and include WebRTC components per Windows WebRTC.
