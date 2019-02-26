#!/usr/bin/env bash
set -xeuo pipefail
./gradlew -PHIFI_ANDROID_PRECOMPILED=${HIFI_ANDROID_PRECOMPILED} -PVERSION_CODE=${VERSION_CODE} -PRELEASE_NUMBER=${RELEASE_NUMBER} -PRELEASE_TYPE=${RELEASE_TYPE} ${ANDROID_APP}:${ANDROID_BUILD_TARGET}

# This is the actual output from gradle, which no longer attempts to muck with the naming of the APK
OUTPUT_APK=./apps/${ANDROID_APP}/build/outputs/apk/${ANDROID_BUILD_DIR}/${ANDROID_BUILT_APK_NAME}
# This is the APK name requested by Jenkins
TARGET_APK=./${ANDROID_APK_NAME}
# Make sure this matches up with the new ARTIFACT_EXPRESSION for jenkins builds, which should be "android/*.apk"
cp ${OUTPUT_APK} ${TARGET_APK}

