//
//  Created by Bradley Austin Davis on 2015/08/08
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OculusHelpers.h"

#include <atomic>

#include <Windows.h>
#include <QtCore/QLoggingCategory>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QProcessEnvironment>

#include <controllers/Input.h>
#include <controllers/Pose.h>
#include <NumericalConstants.h>

using Mutex = std::mutex;
using Lock = std::unique_lock<Mutex>;

Q_DECLARE_LOGGING_CATEGORY(oculus)
Q_LOGGING_CATEGORY(oculus, "hifi.plugins.oculus")

static std::atomic<uint32_t> refCount { 0 };
static ovrSession session { nullptr };

inline ovrErrorInfo getError() {
    ovrErrorInfo error;
    ovr_GetLastErrorInfo(&error);
    return error;
}

void logWarning(const char* what) {
    qWarning(oculus) << what << ":" << getError().ErrorString;
}

void logFatal(const char* what) {
    std::string error("[oculus] ");
    error += what;
    error += ": ";
    error += getError().ErrorString;
    qFatal(error.c_str());
}


static wchar_t* REQUIRED_OCULUS_DLL = L"LibOVRRT64_1.dll";
static wchar_t FOUND_PATH[MAX_PATH];

bool oculusAvailable() {
    static std::once_flag once;
    static bool result { false };
    std::call_once(once, [&] {

        static const QString DEBUG_FLAG("HIFI_DEBUG_OPENVR");
        static bool enableDebugOpenVR = QProcessEnvironment::systemEnvironment().contains(DEBUG_FLAG);
        if (enableDebugOpenVR) {
            return;
        }

        ovrDetectResult detect = ovr_Detect(0);
        if (!detect.IsOculusServiceRunning || !detect.IsOculusHMDConnected) {
            return;
        }

        DWORD searchResult = SearchPathW(NULL, REQUIRED_OCULUS_DLL, NULL, MAX_PATH, FOUND_PATH, NULL);
        if (searchResult <= 0) {
            return;
        }

        result = true;
    });

    return result;
}

ovrSession acquireOculusSession() {
    if (!session && !oculusAvailable()) {
        qCDebug(oculus) << "oculus: no runtime or HMD present";
        return session;
    }

    if (!session) {
        if (!OVR_SUCCESS(ovr_Initialize(nullptr))) {
            logWarning("Failed to initialize Oculus SDK");
            return session;
        }

        Q_ASSERT(0 == refCount);
        ovrGraphicsLuid luid;
        if (!OVR_SUCCESS(ovr_Create(&session, &luid))) {
            logWarning("Failed to acquire Oculus session");
            return session;
        }
    }

    ++refCount;
    return session;
}

void releaseOculusSession() {
    Q_ASSERT(refCount > 0 && session);
    // HACK the Oculus runtime doesn't seem to play well with repeated shutdown / restart.
    // So for now we'll just hold on to the session
#if 0
    if (!--refCount) {
        qCDebug(oculus) << "oculus: zero refcount, shutdown SDK and session";
        ovr_Destroy(session);
        ovr_Shutdown();
        session = nullptr;
    }
#endif
}


controller::Pose ovrControllerPoseToHandPose(
    ovrHandType hand,
    const ovrPoseStatef& handPose) {
    // When the sensor-to-world rotation is identity the coordinate axes look like this:
    //
    //                       user
    //                      forward
    //                        -z
    //                         |
    //                        y|      user
    //      y                  o----x right
    //       o-----x         user
    //       |                up
    //       |
    //       z
    //
    //     Rift

    // From ABOVE the hand canonical axes looks like this:
    //
    //      | | | |          y        | | | |
    //      | | | |          |        | | | |
    //      |     |          |        |     |
    //      |left | /  x---- +      \ |right|
    //      |     _/          z      \_     |
    //       |   |                     |   |
    //       |   |                     |   |
    //

    // So when the user is in Rift space facing the -zAxis with hands outstretched and palms down
    // the rotation to align the Touch axes with those of the hands is:
    //
    //    touchToHand = halfTurnAboutY * quaterTurnAboutX

    // Due to how the Touch controllers fit into the palm there is an offset that is different for each hand.
    // You can think of this offset as the inverse of the measured rotation when the hands are posed, such that
    // the combination (measurement * offset) is identity at this orientation.
    //
    //    Qoffset = glm::inverse(deltaRotation when hand is posed fingers forward, palm down)
    //
    // An approximate offset for the Touch can be obtained by inspection:
    //
    //    Qoffset = glm::inverse(glm::angleAxis(sign * PI/2.0f, zAxis) * glm::angleAxis(PI/4.0f, xAxis))
    //
    // So the full equation is:
    //
    //    Q = combinedMeasurement * touchToHand
    //
    //    Q = (deltaQ * QOffset) * (yFlip * quarterTurnAboutX)
    //
    //    Q = (deltaQ * inverse(deltaQForAlignedHand)) * (yFlip * quarterTurnAboutX)
    static const glm::quat yFlip = glm::angleAxis(PI, Vectors::UNIT_Y);
    static const glm::quat quarterX = glm::angleAxis(PI_OVER_TWO, Vectors::UNIT_X);
    static const glm::quat touchToHand = yFlip * quarterX;

    static const glm::quat leftQuarterZ = glm::angleAxis(-PI_OVER_TWO, Vectors::UNIT_Z);
    static const glm::quat rightQuarterZ = glm::angleAxis(PI_OVER_TWO, Vectors::UNIT_Z);
    static const glm::quat eighthX = glm::angleAxis(PI / 4.0f, Vectors::UNIT_X);

    static const glm::quat leftRotationOffset = glm::inverse(leftQuarterZ * eighthX) * touchToHand;
    static const glm::quat rightRotationOffset = glm::inverse(rightQuarterZ * eighthX) * touchToHand;

    static const float CONTROLLER_LENGTH_OFFSET = 0.0762f;  // three inches
    static const glm::vec3 CONTROLLER_OFFSET = glm::vec3(CONTROLLER_LENGTH_OFFSET / 2.0f,
        CONTROLLER_LENGTH_OFFSET / 2.0f,
        CONTROLLER_LENGTH_OFFSET * 2.0f);
    static const glm::vec3 leftTranslationOffset = glm::vec3(-1.0f, 1.0f, 1.0f) * CONTROLLER_OFFSET;
    static const glm::vec3 rightTranslationOffset = CONTROLLER_OFFSET;

    auto translationOffset = (hand == ovrHand_Left ? leftTranslationOffset : rightTranslationOffset);
    auto rotationOffset = (hand == ovrHand_Left ? leftRotationOffset : rightRotationOffset);

    glm::quat rotation = toGlm(handPose.ThePose.Orientation);

    controller::Pose pose;
    pose.translation = toGlm(handPose.ThePose.Position);
    pose.translation += rotation * translationOffset;
    pose.rotation = rotation * rotationOffset;
    pose.angularVelocity = toGlm(handPose.AngularVelocity);
    pose.velocity = toGlm(handPose.LinearVelocity);
    pose.valid = true;
    return pose;
}