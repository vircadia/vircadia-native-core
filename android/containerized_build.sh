#!/usr/bin/env bash
set -xeuo pipefail

DOCKER_IMAGE_NAME="vircadia_androidbuild"

docker build --build-arg BUILD_UID=`id -u` -t "${DOCKER_IMAGE_NAME}" -f ./android/docker/Dockerfile ./android/docker

# The Jenkins PR builds use VERSION_CODE, but the release builds use VERSION
# So make sure we use VERSION_CODE consistently
test -z "$VERSION_CODE" && export VERSION_CODE=$VERSION

# PR builds don't populate STABLE_BUILD, but the release builds do, and the build
# bash script requires it, so we need to populate it if it's not present
test -z "$STABLE_BUILD" && export STABLE_BUILD=0

# FIXME figure out which of these actually need to be forwarded and which can be eliminated
docker run \
   --rm \
   --security-opt seccomp:unconfined \
   -v "${WORKSPACE}":/home/gha/vircadia \
   -e RELEASE_NUMBER \
   -e RELEASE_TYPE \
   -e ANDROID_APP \
   -e ANDROID_BUILT_APK_NAME \
   -e ANDROID_APK_NAME \
   -e ANDROID_BUILD_TARGET \
   -e ANDROID_BUILD_DIR \
   -e CMAKE_BACKTRACE_URL \
   -e CMAKE_BACKTRACE_TOKEN \
   -e CMAKE_BACKTRACE_SYMBOLS_TOKEN \
   -e GA_TRACKING_ID \
   -e GIT_COMMIT \
   -e OAUTH_CLIENT_SECRET \
   -e OAUTH_CLIENT_ID \
   -e OAUTH_REDIRECT_URI \
   -e GIT_COMMIT_SHORT \
   -e STABLE_BUILD \
   -e VERSION_CODE \
   "${DOCKER_IMAGE_NAME}" \
   sh -c "./build_android.sh"


