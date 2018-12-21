#!/usr/bin/env bash
set -xeuo pipefail

DOCKER_IMAGE_NAME="hifi_androidbuild"

docker build --build-arg BUILD_UID=`id -u` -t "${DOCKER_IMAGE_NAME}" -f docker/Dockerfile docker

docker run \
   --rm \
    --security-opt seccomp:unconfined \
   -v "${WORKSPACE}":/home/jenkins/hifi \
   -e "RELEASE_NUMBER=${RELEASE_NUMBER}" \
   -e "RELEASE_TYPE=${RELEASE_TYPE}" \
   -e "ANDROID_BUILD_TARGET=assembleDebug" \
   -e "CMAKE_BACKTRACE_URL=${CMAKE_BACKTRACE_URL}" \
   -e "CMAKE_BACKTRACE_TOKEN=${CMAKE_BACKTRACE_TOKEN}" \
   -e "CMAKE_BACKTRACE_SYMBOLS_TOKEN=${CMAKE_BACKTRACE_SYMBOLS_TOKEN}" \
   -e "GA_TRACKING_ID=${GA_TRACKING_ID}" \
   -e "GIT_PR_COMMIT=${GIT_PR_COMMIT}" \
   -e "VERSION_CODE=${VERSION_CODE}" \
   "${DOCKER_IMAGE_NAME}" \
   sh -c "./build_android.sh"
