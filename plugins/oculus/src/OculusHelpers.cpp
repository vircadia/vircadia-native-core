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
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QProcessEnvironment>

#define OVRPL_DISABLED
#include <OVR_Platform.h>

#include <controllers/Input.h>
#include <controllers/Pose.h>
#include <shared/GlobalAppProperties.h>
#include <NumericalConstants.h>

Q_LOGGING_CATEGORY(displayplugins, "hifi.plugins.display")
Q_LOGGING_CATEGORY(oculusLog, "hifi.plugins.display.oculus")

using namespace hifi;

static wchar_t* REQUIRED_OCULUS_DLL = L"LibOVRRT64_1.dll";
static wchar_t FOUND_PATH[MAX_PATH];

bool ovr::available() {
    static std::once_flag once;
    static bool result{ false };
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

class ovrImpl {
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;
    std::mutex mutex;
    ovrSession session{ nullptr };
    size_t renderCount{ 0 };

private:
    void setupSession(bool render) {
        if (session) {
            return;
        }
        ovrInitParams initParams{ ovrInit_RequestVersion | ovrInit_FocusAware, OVR_MINOR_VERSION, nullptr, 0, 0 };
        if (render) {
            initParams.Flags |= ovrInit_MixedRendering;
        } else {
            initParams.Flags |= ovrInit_Invisible;
        }

        if (!OVR_SUCCESS(ovr_Initialize(&initParams))) {
            qCWarning(oculusLog) << "Failed to initialze Oculus SDK" << ovr::getError();
            return;
        }

#ifdef OCULUS_APP_ID
        if (qApp->property(hifi::properties::OCULUS_STORE).toBool()) {
            if (ovr_PlatformInitializeWindows(OCULUS_APP_ID) != ovrPlatformInitialize_Success) {
                qCWarning(oculusLog) << "Unable to initialize the platform for entitlement check - fail the check" << ovr::getError();
                return;
            } else {
                qCDebug(oculusLog) << "Performing Oculus Platform entitlement check";
                ovr_Entitlement_GetIsViewerEntitled();
            }
        }
#endif

        ovrGraphicsLuid luid;
        if (!OVR_SUCCESS(ovr_Create(&session, &luid))) {
            qCWarning(oculusLog) << "Failed to acquire Oculus session" << ovr::getError();
            return;
        } else {
            ovrResult setFloorLevelOrigin = ovr_SetTrackingOriginType(session, ovrTrackingOrigin::ovrTrackingOrigin_FloorLevel);
            if (!OVR_SUCCESS(setFloorLevelOrigin)) {
                qCWarning(oculusLog) << "Failed to set the Oculus tracking origin to floor level" << ovr::getError();
            }
        }
    }

    void releaseSession() {
        if (!session) {
            return;
        }
        ovr_Destroy(session);
        session = nullptr;
        ovr_Shutdown();
    }

public:
    void withSession(const std::function<void(ovrSession)>& f) {
        Lock lock(mutex);
        if (!session) {
            setupSession(false);
        }
        f(session);
    }

    ovrSession acquireRenderSession() {
        Lock lock(mutex);
        if (renderCount++ == 0) {
            releaseSession();
            setupSession(true);
        }
        return session;
    }

    void releaseRenderSession(ovrSession session) {
        Lock lock(mutex);
        if (--renderCount == 0) {
            releaseSession();
        }
    }
} _ovr;

ovrSession ovr::acquireRenderSession() {
    return _ovr.acquireRenderSession();
}

void ovr::releaseRenderSession(ovrSession session) {
    _ovr.releaseRenderSession(session);
}

void ovr::withSession(const std::function<void(ovrSession)>& f) {
    _ovr.withSession(f);
}

ovrSessionStatus ovr::getStatus() {
    ovrResult result;
    return ovr::getStatus(result);
}

ovrSessionStatus ovr::getStatus(ovrResult& result) {
    ovrSessionStatus status{};
    withSession([&](ovrSession session) {
        result = ovr_GetSessionStatus(session, &status);
        if (!OVR_SUCCESS(result)) {
            qCWarning(oculusLog) << "Failed to get session status" << ovr::getError();
        }
    });
    return status;
}

ovrTrackingState ovr::getTrackingState(double absTime, ovrBool latencyMarker) {
    ovrTrackingState result{};
    withSession([&](ovrSession session) { result = ovr_GetTrackingState(session, absTime, latencyMarker); });
    return result;
}

QString ovr::getError() {
    static ovrErrorInfo error;
    ovr_GetLastErrorInfo(&error);
    return QString(error.ErrorString);
}

controller::Pose hifi::ovr::toControllerPose(ovrHandType hand, const ovrPoseStatef& handPose) {
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

    static const glm::quat leftRotationOffset = glm::inverse(leftQuarterZ) * touchToHand;
    static const glm::quat rightRotationOffset = glm::inverse(rightQuarterZ) * touchToHand;

    static const float CONTROLLER_LENGTH_OFFSET = 0.0762f;  // three inches
    static const glm::vec3 CONTROLLER_OFFSET =
        glm::vec3(CONTROLLER_LENGTH_OFFSET / 2.0f, -CONTROLLER_LENGTH_OFFSET / 2.0f, CONTROLLER_LENGTH_OFFSET * 1.5f);
    static const glm::vec3 leftTranslationOffset = glm::vec3(-1.0f, 1.0f, 1.0f) * CONTROLLER_OFFSET;
    static const glm::vec3 rightTranslationOffset = CONTROLLER_OFFSET;

    auto translationOffset = (hand == ovrHand_Left ? leftTranslationOffset : rightTranslationOffset);
    auto rotationOffset = (hand == ovrHand_Left ? leftRotationOffset : rightRotationOffset);

    glm::quat rotation = toGlm(handPose.ThePose.Orientation);

    controller::Pose pose;
    pose.translation = toGlm(handPose.ThePose.Position);
    pose.translation += rotation * translationOffset;
    pose.rotation = rotation * rotationOffset;
    pose.angularVelocity = rotation * toGlm(handPose.AngularVelocity);
    pose.velocity = toGlm(handPose.LinearVelocity);
    pose.valid = true;
    return pose;
}

controller::Pose hifi::ovr::toControllerPose(ovrHandType hand,
                                             const ovrPoseStatef& handPose,
                                             const ovrPoseStatef& lastHandPose) {
    static const glm::quat yFlip = glm::angleAxis(PI, Vectors::UNIT_Y);
    static const glm::quat quarterX = glm::angleAxis(PI_OVER_TWO, Vectors::UNIT_X);
    static const glm::quat touchToHand = yFlip * quarterX;

    static const glm::quat leftQuarterZ = glm::angleAxis(-PI_OVER_TWO, Vectors::UNIT_Z);
    static const glm::quat rightQuarterZ = glm::angleAxis(PI_OVER_TWO, Vectors::UNIT_Z);

    static const glm::quat leftRotationOffset = glm::inverse(leftQuarterZ) * touchToHand;
    static const glm::quat rightRotationOffset = glm::inverse(rightQuarterZ) * touchToHand;

    static const float CONTROLLER_LENGTH_OFFSET = 0.0762f;  // three inches
    static const glm::vec3 CONTROLLER_OFFSET =
        glm::vec3(CONTROLLER_LENGTH_OFFSET / 2.0f, -CONTROLLER_LENGTH_OFFSET / 2.0f, CONTROLLER_LENGTH_OFFSET * 1.5f);
    static const glm::vec3 leftTranslationOffset = glm::vec3(-1.0f, 1.0f, 1.0f) * CONTROLLER_OFFSET;
    static const glm::vec3 rightTranslationOffset = CONTROLLER_OFFSET;

    auto translationOffset = (hand == ovrHand_Left ? leftTranslationOffset : rightTranslationOffset);
    auto rotationOffset = (hand == ovrHand_Left ? leftRotationOffset : rightRotationOffset);

    glm::quat rotation = toGlm(handPose.ThePose.Orientation);

    controller::Pose pose;
    pose.translation = toGlm(lastHandPose.ThePose.Position);
    pose.translation += rotation * translationOffset;
    pose.rotation = rotation * rotationOffset;
    pose.angularVelocity = toGlm(lastHandPose.AngularVelocity);
    pose.velocity = toGlm(lastHandPose.LinearVelocity);
    pose.valid = true;
    return pose;
}

bool hifi::ovr::handleOVREvents() {
#ifdef OCULUS_APP_ID
    if (qApp->property(hifi::properties::OCULUS_STORE).toBool()) {
        // pop messages to see if we got a return for an entitlement check
        ovrMessageHandle message = ovr_PopMessage();

        while (message) {
            switch (ovr_Message_GetType(message)) {
                case ovrMessage_Entitlement_GetIsViewerEntitled: {
                    if (!ovr_Message_IsError(message)) {
                        // this viewer is entitled, no need to flag anything
                        qCDebug(oculusLog) << "Oculus Platform entitlement check succeeded, proceeding normally";
                    } else {
                        // we failed the entitlement check, quit
                        qCDebug(oculusLog) << "Oculus Platform entitlement check failed, app will now quit" << OCULUS_APP_ID;
                        return false;
                    }
                }
            }

            // free the message handle to cleanup and not leak
            ovr_FreeMessage(message);

            // pop the next message to check, if there is one
            message = ovr_PopMessage();
        }
    }
#endif
    return true;
}
