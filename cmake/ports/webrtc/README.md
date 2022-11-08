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

## Ubuntu 20.04 - 5387

### Install prerequisite software
```
sudo add-apt-repository ppa:pipewire-debian/pipewire-upstream
sudo apt update
sudo apt install git libssl-dev pkg-config libglib2.0-dev \
  libpipewire-0.3-dev libgbm-dev libegl*-dev libepoxy-dev libdrm-dev clang \
  libasound2-dev libpulse-dev libxdamage-dev libxrandr-dev libxtst-dev \
  libxcomposite-dev
```

### Setup depot_tools

Clone depot_tools repository and set PATH.
```
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=$HOME/depot_tools:$PATH
```

### Setup WebRTC project
Fetch it with depot tools:
```
fetch webrtc
```
This will create an `src` directory, from this point on we will be working there:
```
cd src
```
Checkout the relevant release branch:
```
git checkout branch-heads/5387
```
Synchronize the third party dependencies with depot tools:
```
gclient sync
```

#### BoringSLL replacement
Remove BoringSSL library to make sure it's not included in the build:
```
rm -rf third_party/boringssl
```
This will break several other dependencies, so they need to be updated to use the system OpenSSL library instead of BoringSSL. The commands below that generate build configurations will give errors for relevant third party sub projects. Each of these has a BUILD.gn file in its root, this is configuration file that specifies dependency on boringssl in arrays called `deps` with a path string `//third_party/boringssl`. All those need to be removed. Additionally under `config("<projectname>_config")` OpenSSL libraries linkage parameters need to be added, these are specified in another array called `libs`, that should include `"ssl", "crypto"` to link against libssl and libcrypto. In version 5387 affected third party projects are `grpc` and `libsrtp`, and necessary modification are available in a diff format [here](./linux/patches/5387/replace_boringssl.diff).

#### Building the library

Generate release and debug build configurations:
```
gn gen out/Release --args='use_custom_libcxx=false target_sysroot="/" rtc_include_tests=false rtc_build_examples=false use_debug_fission=false is_debug=false use_rtti=true rtc_use_h264=true proprietary_codecs=true rtc_build_ssl=false rtc_ssl_root=" "'

gn gen out/Debug --args='use_custom_libcxx=false target_sysroot="/" rtc_include_tests=false rtc_build_examples=false use_debug_fission=false is_debug=true rtc_use_h264=true proprietary_codecs=true rtc_build_ssl=false rtc_ssl_root=" "'
```
Build release and debug static libraries:
```
PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig/ ninja -C out/Release
PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig/ ninja -C out/Debug
```

### Assembling the VCPKG package
The VCPKG is a tar.xz archive with the following directories:
* webrtc/lib/ - contains release static library
* webrtc/debug/lib/ - contains debug static library
* webrtc/share/webrtc - contains copyright file
* webrtc/include/webrtc - contains library header files

The release and debug static libraries can be copied over from `src/out/Release/obj/libwebrtc.a` and `src/out/Debug/obj/libwebrtc.a` respectivly. As a copyright file the `src/LICENSE` is copied. For the include firectory all the headers files from the following directories need to be copied over form the `src` directory, preserving the structure: api, rtc_base, modules, system_wrappers, common_video, common_audio, call, media, video, p2p, logging. Additionally all the headers files from `src/third_party/abseil-cpp` also need to be copied, also presercing the strecture.

### Automation

The [linux](./linux) directory contains a Makefile and patches to automate the entire process from installing prerequisits to creating the VCPKG package, tested on Ubuntu 20.04 server, WebRTC version 5387. To use it copy the contents to a folder to where WebRTC should be built and invoke `make`. The Makefile has the following parameters:
* VERSION - the version of webrtc to checkout, this is just the number, will be prefixed with "branch_heads/" for the git checkout command. (default: 5387)
* DESTDIR - the output directory in which to build the package VCPKG package (default: package)
If the process completes succsessfully the VCPKG package will be available under `DESTDIR` as `webrtc-VERSION-linux.tar.xz`. The [patches](./linux/patches) directory contains a directory per version which contain diff files to apply to `src` folder berfore building. This way if different changes are required for future versions of WebRTC they can be added there as diff files under appropriate directories.

## MacOS - M78

The original, High Fidelity-provided WebRTC VCPKG library is used for AEC (audio echo cancellation) only.

**TODO:** Update to M84 and include WebRTC components per Windows WebRTC.
