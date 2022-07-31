# https://developer.android.com/ndk/guides/other_build_systems
export TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/linux-x86_64

# export TARGET=aarch64-linux-android
export TARGET=armv7a-linux-androideabi
# export TARGET=i686-linux-android
# export TARGET=x86_64-linux-android

# Set this to your minSdkVersion.
export API=21

# Configure and build.
export AR=$TOOLCHAIN/bin/llvm-ar
export CC=$TOOLCHAIN/bin/$TARGET$API-clang
export AS=$CC
export CXX=$TOOLCHAIN/bin/$TARGET$API-clang++
export LD=$TOOLCHAIN/bin/ld
export CCLD=CC
export RANLIB=$TOOLCHAIN/bin/llvm-ranlib
export STRIP=$TOOLCHAIN/bin/llvm-strip
echo Destination $(pwd)/opus_package
../configure --host $TARGET --target $TARGET$API --prefix $(pwd)/opus_package --disable-silent-rules --disable-extra-programs --disable-shared
make install -j4

