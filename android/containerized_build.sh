#!/usr/bin/env bash
set -xeuo pipefail

DOCKER_IMAGE_NAME="hifi_androidbuild"

docker build --build-arg BUILD_UID=`id -u` -t "${DOCKER_IMAGE_NAME}" -f docker/Dockerfile docker

docker run \
   --rm \
    --security-opt seccomp:unconfined \
   -v "${WORKSPACE}":/home/jenkins/hifi \
   -e RELEASE_NUMBER \
   -e RELEASE_TYPE \
   -e ANDROID_BUILD_TARGET \
   -e ANDROID_BUILD_DIR \
   -e CMAKE_BACKTRACE_URL \
   -e CMAKE_BACKTRACE_TOKEN \
   -e CMAKE_BACKTRACE_SYMBOLS_TOKEN \
   -e GA_TRACKING_ID \
   -e OAUTH_CLIENT_SECRET \
   -e OAUTH_CLIENT_ID \
   -e OAUTH_REDIRECT_URI \
   -e VERSION_CODE \
   "${DOCKER_IMAGE_NAME}" \
   sh -c "./build_android.sh"


