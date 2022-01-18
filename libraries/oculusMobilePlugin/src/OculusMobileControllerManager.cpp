//
//  Created by Bradley Austin Davis on 2018/11/15
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OculusMobileControllerManager.h"

#include <array>

#include <ui-plugins/PluginContainer.h>
#include <controllers/UserInputMapper.h>
#include <controllers/StandardControls.h>
#include <input-plugins/KeyboardMouseDevice.h>

#include <PerfStat.h>
#include <PathUtils.h>
#include <NumericalConstants.h>
#include <StreamUtils.h>
#include <VrApi.h>
#include <VrApi_Input.h>
#include <ovr/Helpers.h>

#include "Logging.h"
#include <ovr/VrHandler.h>

const char* OculusMobileControllerManager::NAME = "Oculus";
const quint64 LOST_TRACKING_DELAY = 3000000;

namespace ovr {

    controller::Pose toControllerPose(ovrTrackedDeviceTypeId hand, const ovrRigidBodyPosef& handPose) {
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

        auto translationOffset = (hand == VRAPI_HAND_LEFT ? leftTranslationOffset : rightTranslationOffset);
        auto rotationOffset = (hand == VRAPI_HAND_LEFT ? leftRotationOffset : rightRotationOffset);

        glm::quat rotation = toGlm(handPose.Pose.Orientation);

        controller::Pose pose;
        pose.translation = toGlm(handPose.Pose.Position);
        pose.translation += rotation * translationOffset;
        pose.rotation = rotation * rotationOffset;
        pose.angularVelocity = rotation * toGlm(handPose.AngularVelocity);
        pose.velocity = toGlm(handPose.LinearVelocity);
        pose.valid = true;
        return pose;
    }

    controller::Pose toControllerPose(ovrTrackedDeviceTypeId hand,
                                      const ovrRigidBodyPosef& handPose,
                                      const ovrRigidBodyPosef& lastHandPose) {
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

        auto translationOffset = (hand == VRAPI_HAND_LEFT ? leftTranslationOffset : rightTranslationOffset);
        auto rotationOffset = (hand == VRAPI_HAND_LEFT ? leftRotationOffset : rightRotationOffset);

        glm::quat rotation = toGlm(handPose.Pose.Orientation);

        controller::Pose pose;
        pose.translation = toGlm(lastHandPose.Pose.Position);
        pose.translation += rotation * translationOffset;
        pose.rotation = rotation * rotationOffset;
        pose.angularVelocity = toGlm(lastHandPose.AngularVelocity);
        pose.velocity = toGlm(lastHandPose.LinearVelocity);
        pose.valid = true;
        return pose;
    }

}


class OculusMobileInputDevice : public controller::InputDevice {
    friend class OculusMobileControllerManager;
public:
    using Pointer = std::shared_ptr<OculusMobileInputDevice>;
    static Pointer check(ovrMobile* session);

    OculusMobileInputDevice(ovrMobile* session, const std::vector<ovrInputTrackedRemoteCapabilities>& devicesCaps);
    void updateHands(ovrMobile* session);

    controller::Input::NamedVector getAvailableInputs() const override;
    QString getDefaultMappingConfig() const override;
    void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;
    void focusOutEvent() override;
    bool triggerHapticPulse(float strength, float duration, uint16_t index) override;

private:
    void handlePose(float deltaTime, const controller::InputCalibrationData& inputCalibrationData,
                    ovrTrackedDeviceTypeId hand, const ovrRigidBodyPosef& handPose);
    void handleRotationForUntrackedHand(const controller::InputCalibrationData& inputCalibrationData,
                                        ovrTrackedDeviceTypeId hand, const ovrRigidBodyPosef& handPose);
    void handleHeadPose(float deltaTime, const controller::InputCalibrationData& inputCalibrationData,
                        const ovrRigidBodyPosef& headPose);

    void reconnectTouchControllers(ovrMobile* session);

    // perform an action when the TouchDevice mutex is acquired.
    using Locker = std::unique_lock<std::recursive_mutex>;

    template <typename F>
    void withLock(F&& f) { Locker locker(_lock); f(); }

    mutable std::recursive_mutex _lock;
    ovrTracking2 _headTracking;
    struct HandData {
        HandData() {
            state.Header.ControllerType =ovrControllerType_TrackedRemote;
        }

        float hapticDuration { 0.0f };
        float hapticStrength { 0.0f };
        bool valid{ false };
        bool lostTracking{ false };
        quint64 regainTrackingDeadline;
        ovrRigidBodyPosef lastPose;
        ovrInputTrackedRemoteCapabilities caps;
        ovrInputStateTrackedRemote state;
        ovrResult stateResult{ ovrError_NotInitialized };
        ovrTracking tracking;
        ovrResult trackingResult{ ovrError_NotInitialized };
        bool setHapticFeedback(float strength, float duration) {
#if 0
            bool success = true;
            bool sessionSuccess = ovr::VrHandler::withOvrMobile([&](ovrMobile* session){
                if (strength == 0.0f) {
                    hapticStrength = 0.0f;
                    hapticDuration = 0.0f;
                } else {
                    hapticStrength = (duration > hapticDuration) ? strength : hapticStrength;
                    if (vrapi_SetHapticVibrationSimple(session, caps.Header.DeviceID, hapticStrength) != ovrSuccess) {
                        success = false;
                    }
                    hapticDuration = std::max(duration, hapticDuration);
                }
            });
            return success && sessionSuccess;
#else
            return true;
#endif
        }

        void stopHapticPulse() {
            ovr::VrHandler::withOvrMobile([&](ovrMobile* session){
                vrapi_SetHapticVibrationSimple(session, caps.Header.DeviceID, 0.0f);
            });
        }

        bool isValid() const {
            return (stateResult == ovrSuccess) && (trackingResult == ovrSuccess);
        }

        void update(ovrMobile* session, double time = 0.0) {
            const auto& deviceId = caps.Header.DeviceID;
            stateResult = vrapi_GetCurrentInputState(session, deviceId, &state.Header);
            trackingResult = vrapi_GetInputTrackingState(session, deviceId, 0.0, &tracking);
        }
    };
    std::array<HandData, 2> _hands;
};

OculusMobileInputDevice::Pointer OculusMobileInputDevice::check(ovrMobile *session) {
    Pointer result;

    std::vector<ovrInputTrackedRemoteCapabilities> devicesCaps;
    {
        uint32_t deviceIndex { 0 };
        ovrInputCapabilityHeader capsHeader;
        while (vrapi_EnumerateInputDevices(session, deviceIndex, &capsHeader) >= 0) {
            if (capsHeader.Type == ovrControllerType_TrackedRemote) {
                ovrInputTrackedRemoteCapabilities caps;
                caps.Header = capsHeader;
                vrapi_GetInputDeviceCapabilities(session, &caps.Header);
                devicesCaps.push_back(caps);
            }
            ++deviceIndex;
        }
    }
    if (!devicesCaps.empty()) {
        result.reset(new OculusMobileInputDevice(session, devicesCaps));
    }
    return result;
}

static OculusMobileInputDevice::Pointer oculusMobileControllers;

bool OculusMobileControllerManager::isHandController() const {
    return oculusMobileControllers.operator bool();
}

bool OculusMobileControllerManager::isSupported() const {
    return true;
}

bool OculusMobileControllerManager::activate() {
    InputPlugin::activate();
    checkForConnectedDevices();
    return true;
}

void OculusMobileControllerManager::checkForConnectedDevices() {
    if (oculusMobileControllers) {
        return;
    }

    ovr::VrHandler::withOvrMobile([&](ovrMobile* session){
        oculusMobileControllers = OculusMobileInputDevice::check(session);
        if (oculusMobileControllers) {
            auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
            userInputMapper->registerDevice(oculusMobileControllers);
        }
    });
}

void OculusMobileControllerManager::deactivate() {
    InputPlugin::deactivate();

    // unregister with UserInputMapper
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    if (oculusMobileControllers) {
        userInputMapper->removeDevice(oculusMobileControllers->getDeviceID());
        oculusMobileControllers.reset();
    }
}

void OculusMobileControllerManager::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    PerformanceTimer perfTimer("OculusMobileInputDevice::update");

    checkForConnectedDevices();

    if (!oculusMobileControllers) {
        return;
    }

    bool updated = ovr::VrHandler::withOvrMobile([&](ovrMobile* session){
        oculusMobileControllers->updateHands(session);
    });

    if (updated) {
        oculusMobileControllers->update(deltaTime, inputCalibrationData);
    }
}

void OculusMobileControllerManager::pluginFocusOutEvent() {
    if (oculusMobileControllers) {
        oculusMobileControllers->focusOutEvent();
    }
}

QStringList OculusMobileControllerManager::getSubdeviceNames() {
    QStringList devices;
    if (oculusMobileControllers) {
        devices << oculusMobileControllers->getName();
    }
    return devices;
}

using namespace controller;

static const std::vector<std::pair<ovrButton, StandardButtonChannel>> BUTTON_MAP { {
    { ovrButton_Up, DU },
    { ovrButton_Down, DD },
    { ovrButton_Left, DL },
    { ovrButton_Right, DR },
    { ovrButton_Enter, START },
    { ovrButton_Back, BACK },
    { ovrButton_X, X },
    { ovrButton_Y, Y },
    { ovrButton_A, A },
    { ovrButton_B, B },
    { ovrButton_LThumb, LS },
    { ovrButton_RThumb, RS },
    //{ ovrButton_LShoulder, LB },
    //{ ovrButton_RShoulder, RB },
} };

static const std::vector<std::pair<ovrTouch, StandardButtonChannel>> LEFT_TOUCH_MAP { {
    { ovrTouch_X, LEFT_PRIMARY_THUMB_TOUCH },
    { ovrTouch_Y, LEFT_SECONDARY_THUMB_TOUCH },
    { ovrTouch_LThumb, LS_TOUCH },
    { ovrTouch_ThumbUp, LEFT_THUMB_UP },
    { ovrTouch_IndexTrigger, LEFT_PRIMARY_INDEX_TOUCH },
    { ovrTouch_IndexPointing, LEFT_INDEX_POINT },
} };


static const std::vector<std::pair<ovrTouch, StandardButtonChannel>> RIGHT_TOUCH_MAP { {
    { ovrTouch_A, RIGHT_PRIMARY_THUMB_TOUCH },
    { ovrTouch_B, RIGHT_SECONDARY_THUMB_TOUCH },
    { ovrTouch_RThumb, RS_TOUCH },
    { ovrTouch_ThumbUp, RIGHT_THUMB_UP },
    { ovrTouch_IndexTrigger, RIGHT_PRIMARY_INDEX_TOUCH },
    { ovrTouch_IndexPointing, RIGHT_INDEX_POINT },
} };

void OculusMobileInputDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    _buttonPressedMap.clear();

    int numTrackedControllers = 0;
    quint64 currentTime = usecTimestampNow();
    handleHeadPose(deltaTime, inputCalibrationData, _headTracking.HeadPose);

    static const auto REQUIRED_HAND_STATUS = VRAPI_TRACKING_STATUS_ORIENTATION_TRACKED | VRAPI_TRACKING_STATUS_POSITION_TRACKED;
    ovr::for_each_hand([&](ovrTrackedDeviceTypeId hand) {
        size_t handIndex = (hand == VRAPI_TRACKED_DEVICE_HAND_LEFT) ? 0 : 1;
        int controller = (hand == VRAPI_TRACKED_DEVICE_HAND_LEFT) ? controller::LEFT_HAND : controller::RIGHT_HAND;
        auto& handData = _hands[handIndex];
        const auto& tracking = handData.tracking;
        ++numTrackedControllers;

        // Disable hand tracking while in Oculus Dash (Dash renders it's own hands)
//        if (!hasInputFocus) {
//            _poseStateMap.erase(controller);
//            _poseStateMap[controller].valid = false;
//            return;
//        }

        if (REQUIRED_HAND_STATUS == (tracking.Status & REQUIRED_HAND_STATUS)) {
            _poseStateMap.erase(controller);
            handlePose(deltaTime, inputCalibrationData, hand, tracking.HeadPose);
            handData.lostTracking = false;
            handData.lastPose = tracking.HeadPose;
            return;
        }

        if (handData.lostTracking) {
            if (currentTime > handData.regainTrackingDeadline) {
                _poseStateMap.erase(controller);
                _poseStateMap[controller].valid = false;
                return;
            }
        } else {
            quint64 deadlineToRegainTracking = currentTime + LOST_TRACKING_DELAY;
            handData.regainTrackingDeadline = deadlineToRegainTracking;
            handData.lostTracking = true;
        }
        handleRotationForUntrackedHand(inputCalibrationData, hand, tracking.HeadPose);
    });


    using namespace controller;
    // Axes
    {
        const auto& inputState = _hands[0].state;
        _axisStateMap[LX].value = inputState.JoystickNoDeadZone.x;
        _axisStateMap[LY].value = inputState.JoystickNoDeadZone.y;
        _axisStateMap[LT].value = inputState.IndexTrigger;
        _axisStateMap[LEFT_GRIP].value = inputState.GripTrigger;
        for (const auto& pair : BUTTON_MAP) {
            if (inputState.Buttons & pair.first) {
                _buttonPressedMap.insert(pair.second);
                qDebug()<<"AAAA:BUTTON PRESSED "<<pair.second;
            }
        }
        for (const auto& pair : LEFT_TOUCH_MAP) {
            if (inputState.Touches & pair.first) {
                _buttonPressedMap.insert(pair.second);
            }
        }
    }

    {
        const auto& inputState = _hands[1].state;
        _axisStateMap[RX].value = inputState.JoystickNoDeadZone.x;
        _axisStateMap[RY].value = inputState.JoystickNoDeadZone.y;
        _axisStateMap[RT].value = inputState.IndexTrigger;
        _axisStateMap[RIGHT_GRIP].value = inputState.GripTrigger;

        for (const auto& pair : BUTTON_MAP) {
            if (inputState.Buttons & pair.first) {
                _buttonPressedMap.insert(pair.second);
            }
        }
        for (const auto& pair : RIGHT_TOUCH_MAP) {
            if (inputState.Touches & pair.first) {
                _buttonPressedMap.insert(pair.second);
            }
        }
    }

    // Haptics
    {
        Locker locker(_lock);
        for (auto& hand : _hands) {
            if (hand.hapticDuration) {
                hand.hapticDuration -= deltaTime * 1000.0f; // milliseconds
            } else {
                hand.stopHapticPulse();
            }
        }
    }
}

void OculusMobileInputDevice::focusOutEvent() {
    _axisStateMap.clear();
    _buttonPressedMap.clear();
};

void OculusMobileInputDevice::handlePose(float deltaTime,
                                                      const controller::InputCalibrationData& inputCalibrationData,
                                                      ovrTrackedDeviceTypeId hand, const ovrRigidBodyPosef& handPose) {
    auto poseId = (hand == VRAPI_HAND_LEFT) ? controller::LEFT_HAND : controller::RIGHT_HAND;
    auto& pose = _poseStateMap[poseId];
    pose = ovr::toControllerPose(hand, handPose);
    // transform into avatar frame
    glm::mat4 controllerToAvatar = glm::inverse(inputCalibrationData.avatarMat) * inputCalibrationData.sensorToWorldMat;
    pose = pose.transform(controllerToAvatar);
}

void OculusMobileInputDevice::handleHeadPose(float deltaTime,
                                                          const controller::InputCalibrationData& inputCalibrationData,
                                                          const ovrRigidBodyPosef& headPose) {
    glm::mat4 mat = createMatFromQuatAndPos(ovr::toGlm(headPose.Pose.Orientation),
                                            ovr::toGlm(headPose.Pose.Position));

    //perform a 180 flip to make the HMD face the +z instead of -z, beacuse the head faces +z
    glm::mat4 matYFlip = mat * Matrices::Y_180;
    controller::Pose pose(extractTranslation(matYFlip),
                          glmExtractRotation(matYFlip),
                          ovr::toGlm(headPose.LinearVelocity), // XXX * matYFlip ?
                          ovr::toGlm(headPose.AngularVelocity));

    glm::mat4 sensorToAvatar = glm::inverse(inputCalibrationData.avatarMat) * inputCalibrationData.sensorToWorldMat;
    glm::mat4 defaultHeadOffset = glm::inverse(inputCalibrationData.defaultCenterEyeMat) *
        inputCalibrationData.defaultHeadMat;

    pose.valid = true;
    _poseStateMap[controller::HEAD] = pose.postTransform(defaultHeadOffset).transform(sensorToAvatar);
}

void OculusMobileInputDevice::handleRotationForUntrackedHand(const controller::InputCalibrationData& inputCalibrationData,
                                                                          ovrTrackedDeviceTypeId hand, const ovrRigidBodyPosef& handPose) {
    auto poseId = (hand == VRAPI_HAND_LEFT ? controller::LEFT_HAND : controller::RIGHT_HAND);
    auto& pose = _poseStateMap[poseId];
    const auto& lastHandPose = (hand == VRAPI_HAND_LEFT) ? _hands[0].lastPose : _hands[1].lastPose;
    pose = ovr::toControllerPose(hand, handPose, lastHandPose);
    glm::mat4 controllerToAvatar = glm::inverse(inputCalibrationData.avatarMat) * inputCalibrationData.sensorToWorldMat;
    pose = pose.transform(controllerToAvatar);
}

bool OculusMobileInputDevice::triggerHapticPulse(float strength, float duration, uint16_t index) {
    if (index > 2) {
        return false;
    }

    controller::Hand hand = (controller::Hand)index;

    Locker locker(_lock);
    bool success = true;

    if (hand == controller::BOTH || hand == controller::LEFT) {
        success &= _hands[0].setHapticFeedback(strength, duration);
    }
    if (hand == controller::BOTH || hand == controller::RIGHT) {
        success &= _hands[0].setHapticFeedback(strength, duration);
    }
    return success;
}

/*@jsdoc
 * <p>The <code>Controller.Hardware.OculusTouch</code> object has properties representing Oculus Rift. The property values are 
 * integer IDs, uniquely identifying each output. <em>Read-only.</em> These can be mapped to actions or functions or 
 * <code>Controller.Standard</code> items in a {@link RouteObject} mapping.</p>
 * <table>
 *   <thead>
 *     <tr><th>Property</th><th>Type</th><th>Data</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td colspan="4"><strong>Buttons</strong></td></tr>
 *     <tr><td><code>A</code></td><td>number</td><td>number</td><td>"A" button pressed.</td></tr>
 *     <tr><td><code>B</code></td><td>number</td><td>number</td><td>"B" button pressed.</td></tr>
 *     <tr><td><code>X</code></td><td>number</td><td>number</td><td>"X" button pressed.</td></tr>
 *     <tr><td><code>Y</code></td><td>number</td><td>number</td><td>"Y" button pressed.</td></tr>
 *     <tr><td><code>LeftApplicationMenu</code></td><td>number</td><td>number</td><td>Left application menu button pressed.
 *       </td></tr>
 *     <tr><td><code>RightApplicationMenu</code></td><td>number</td><td>number</td><td>Right application menu button pressed.
 *       </td></tr>
 *     <tr><td colspan="4"><strong>Sticks</strong></td></tr>
 *     <tr><td><code>LX</code></td><td>number</td><td>number</td><td>Left stick x-axis scale.</td></tr>
 *     <tr><td><code>LY</code></td><td>number</td><td>number</td><td>Left stick y-axis scale.</td></tr>
 *     <tr><td><code>RX</code></td><td>number</td><td>number</td><td>Right stick x-axis scale.</td></tr>
 *     <tr><td><code>RY</code></td><td>number</td><td>number</td><td>Right stick y-axis scale.</td></tr>
 *     <tr><td><code>LS</code></td><td>number</td><td>number</td><td>Left stick button pressed.</td></tr>
 *     <tr><td><code>RS</code></td><td>number</td><td>number</td><td>Right stick button pressed.</td></tr>
 *     <tr><td><code>LSTouch</code></td><td>number</td><td>number</td><td>Left stick is touched.</td></tr>
 *     <tr><td><code>RSTouch</code></td><td>number</td><td>number</td><td>Right stick is touched.</td></tr>
 *     <tr><td colspan="4"><strong>Triggers</strong></td></tr>
 *     <tr><td><code>LT</code></td><td>number</td><td>number</td><td>Left trigger scale.</td></tr>
 *     <tr><td><code>RT</code></td><td>number</td><td>number</td><td>Right trigger scale.</td></tr>
 *     <tr><td><code>LeftGrip</code></td><td>number</td><td>number</td><td>Left grip scale.</td></tr>
 *     <tr><td><code>RightGrip</code></td><td>number</td><td>number</td><td>Right grip scale.</td></tr>
 *     <tr><td colspan="4"><strong>Finger Abstractions</strong></td></tr>
 *     <tr><td><code>LeftPrimaryThumbTouch</code></td><td>number</td><td>number</td><td>Left thumb touching primary thumb 
 *       button.</td></tr>
 *     <tr><td><code>LeftSecondaryThumbTouch</code></td><td>number</td><td>number</td><td>Left thumb touching secondary thumb 
 *       button.</td></tr>
 *     <tr><td><code>LeftThumbUp</code></td><td>number</td><td>number</td><td>Left thumb not touching primary or secondary 
 *       thumb buttons.</td></tr>
 *     <tr><td><code>RightPrimaryThumbTouch</code></td><td>number</td><td>number</td><td>Right thumb touching primary thumb 
 *       button.</td></tr>
 *     <tr><td><code>RightSecondaryThumbTouch</code></td><td>number</td><td>number</td><td>Right thumb touching secondary thumb 
 *       button.</td></tr>
 *     <tr><td><code>RightThumbUp</code></td><td>number</td><td>number</td><td>Right thumb not touching primary or secondary 
 *       thumb buttons.</td></tr>
 *     <tr><td><code>LeftPrimaryIndexTouch</code></td><td>number</td><td>number</td><td>Left index finger is touching primary 
 *       index finger control.</td></tr>
 *     <tr><td><code>LeftIndexPoint</code></td><td>number</td><td>number</td><td>Left index finger is pointing, not touching 
 *       primary or secondary index finger controls.</td></tr>
 *     <tr><td><code>RightPrimaryIndexTouch</code></td><td>number</td><td>number</td><td>Right index finger is touching primary 
 *       index finger control.</td></tr>
 *     <tr><td><code>RightIndexPoint</code></td><td>number</td><td>number</td><td>Right index finger is pointing, not touching 
 *       primary or secondary index finger controls.</td></tr>
 *     <tr><td colspan="4"><strong>Avatar Skeleton</strong></td></tr>
 *     <tr><td><code>Head</code></td><td>number</td><td>{@link Pose}</td><td>Head pose.</td></tr>
 *     <tr><td><code>LeftHand</code></td><td>number</td><td>{@link Pose}</td><td>Left hand pose.</td></tr>
 *     <tr><td><code>RightHand</code></td><td>number</td><td>{@link Pose}</td><td>right hand pose.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {object} Controller.Hardware-OculusTouch
 */
controller::Input::NamedVector OculusMobileInputDevice::getAvailableInputs() const {
    using namespace controller;
    QVector<Input::NamedPair> availableInputs{
        // buttons
        makePair(A, "A"),
        makePair(B, "B"),
        makePair(X, "X"),
        makePair(Y, "Y"),

        // trackpad analogs
        makePair(LX, "LX"),
        makePair(LY, "LY"),
        makePair(RX, "RX"),
        makePair(RY, "RY"),

        // triggers
        makePair(LT, "LT"),
        makePair(RT, "RT"),

        // trigger buttons
        //makePair(LB, "LB"),
        //makePair(RB, "RB"),

        // side grip triggers
        makePair(LEFT_GRIP, "LeftGrip"),
        makePair(RIGHT_GRIP, "RightGrip"),

        // joystick buttons
        makePair(LS, "LS"),
        makePair(RS, "RS"),

        makePair(LEFT_HAND, "LeftHand"),
        makePair(RIGHT_HAND, "RightHand"),
        makePair(HEAD, "Head"),

        makePair(LEFT_PRIMARY_THUMB_TOUCH, "LeftPrimaryThumbTouch"),
        makePair(LEFT_SECONDARY_THUMB_TOUCH, "LeftSecondaryThumbTouch"),
        makePair(RIGHT_PRIMARY_THUMB_TOUCH, "RightPrimaryThumbTouch"),
        makePair(RIGHT_SECONDARY_THUMB_TOUCH, "RightSecondaryThumbTouch"),
        makePair(LEFT_PRIMARY_INDEX_TOUCH, "LeftPrimaryIndexTouch"),
        makePair(RIGHT_PRIMARY_INDEX_TOUCH, "RightPrimaryIndexTouch"),
        makePair(LS_TOUCH, "LSTouch"),
        makePair(RS_TOUCH, "RSTouch"),
        makePair(LEFT_THUMB_UP, "LeftThumbUp"),
        makePair(RIGHT_THUMB_UP, "RightThumbUp"),
        makePair(LEFT_INDEX_POINT, "LeftIndexPoint"),
        makePair(RIGHT_INDEX_POINT, "RightIndexPoint"),
        makePair(BACK, "LeftApplicationMenu"),
        makePair(START, "RightApplicationMenu"),
    };
    return availableInputs;
}

OculusMobileInputDevice::OculusMobileInputDevice(ovrMobile* session, const std::vector<ovrInputTrackedRemoteCapabilities>& devicesCaps) : controller::InputDevice("OculusTouch") {
    qWarning() << "QQQ" << __FUNCTION__ << "Found " << devicesCaps.size() << "devices";
    for (const auto& deviceCaps : devicesCaps) {
        size_t handIndex = -1;
        if (deviceCaps.ControllerCapabilities & ovrControllerCaps_LeftHand) {
            handIndex = 0;
        } else if (deviceCaps.ControllerCapabilities & ovrControllerCaps_RightHand) {
            handIndex = 1;
        } else {
            continue;
        }
        HandData& handData = _hands[handIndex];
        handData.state.Header.ControllerType = ovrControllerType_TrackedRemote;
        handData.valid = true;
        handData.caps = deviceCaps;
        handData.update(session);
    }
}

void OculusMobileInputDevice::updateHands(ovrMobile* session) {
    _headTracking = vrapi_GetPredictedTracking2(session, 0.0);

    bool touchControllerNotConnected = false;
    for (auto& hand : _hands) {
        hand.update(session);

        if (hand.stateResult < 0 || hand.trackingResult < 0) {
            touchControllerNotConnected = true;
        }
    }

    if (touchControllerNotConnected) {
        reconnectTouchControllers(session);
    }
}

void OculusMobileInputDevice::reconnectTouchControllers(ovrMobile* session) {
    uint32_t deviceIndex { 0 };
    ovrInputCapabilityHeader capsHeader;
    while (vrapi_EnumerateInputDevices(session, deviceIndex, &capsHeader) >= 0) {
        if (capsHeader.Type == ovrControllerType_TrackedRemote) {
            ovrInputTrackedRemoteCapabilities caps;
            caps.Header = capsHeader;
            vrapi_GetInputDeviceCapabilities(session, &caps.Header);

            if (caps.ControllerCapabilities & ovrControllerCaps_LeftHand || caps.ControllerCapabilities & ovrControllerCaps_RightHand) {
                size_t handIndex = caps.ControllerCapabilities & ovrControllerCaps_LeftHand ? 0 : 1;
                HandData& handData = _hands[handIndex];
                handData.state.Header.ControllerType = ovrControllerType_TrackedRemote;
                handData.valid = true;
                handData.caps = caps;
                handData.update(session);
            }
        }
        ++deviceIndex;
    }
}

QString OculusMobileInputDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/oculus_touch.json";
    return MAPPING_JSON;
}

// TODO migrate to a DLL model where plugins are discovered and loaded at runtime by the PluginManager class
InputPluginList getInputPlugins() {
    InputPlugin* PLUGIN_POOL[] = {
        new KeyboardMouseDevice(),
        new OculusMobileControllerManager(),
        nullptr
    };

    InputPluginList result;
    for (int i = 0; PLUGIN_POOL[i]; ++i) {
        InputPlugin* plugin = PLUGIN_POOL[i];
        if (plugin->isSupported()) {
            result.push_back(InputPluginPointer(plugin));
        }
    }
    return result;
}
