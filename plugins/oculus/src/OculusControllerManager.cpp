//
//  OculusControllerManager.cpp
//  input-plugins/src/input-plugins
//
//  Created by Bradley Austin Davis 2016/03/04.
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OculusControllerManager.h"

#include <QtCore/QLoggingCategory>

#include <ui-plugins/PluginContainer.h>
#include <controllers/UserInputMapper.h>
#include <controllers/StandardControls.h>

#include <AvatarConstants.h>
#include <PerfStat.h>
#include <PathUtils.h>
#include <NumericalConstants.h>
#include <StreamUtils.h>

#include <ovr_capi.h>

#include "OculusHelpers.h"

using namespace hifi;

const char* OculusControllerManager::NAME = "Oculus";
const QString OCULUS_LAYOUT = "OculusConfiguration.qml";

const quint64 LOST_TRACKING_DELAY = 3000000;

bool OculusControllerManager::isSupported() const {
    return hifi::ovr::available();
}

bool OculusControllerManager::activate() {
    InputPlugin::activate();
    checkForConnectedDevices();
    return true;
}

QString OculusControllerManager::configurationLayout() {
    return OCULUS_LAYOUT;
}

void OculusControllerManager::setConfigurationSettings(const QJsonObject configurationSettings) {
    if (configurationSettings.contains("trackControllersInOculusHome")) {
        _trackControllersInOculusHome.set(configurationSettings["trackControllersInOculusHome"].toBool());
    }
}

QJsonObject OculusControllerManager::configurationSettings() {
    QJsonObject configurationSettings;
    configurationSettings["trackControllersInOculusHome"] = _trackControllersInOculusHome.get();
    return configurationSettings;
}

void OculusControllerManager::checkForConnectedDevices() {
    if (_touch && _remote) {
        return;
    }

    ovr::withSession([&] (ovrSession session) {
        unsigned int controllerConnected = ovr_GetConnectedControllerTypes(session);

        if (!_remote && (controllerConnected & ovrControllerType_Remote) == ovrControllerType_Remote) {
            if (OVR_SUCCESS(ovr_GetInputState(session, ovrControllerType_Remote, &_remoteInputState))) {
                auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
                _remote = std::make_shared<RemoteDevice>(*this);
                userInputMapper->registerDevice(_remote);
            }
        }

        if (!_touch && (controllerConnected & ovrControllerType_Touch) != 0) {
            if (OVR_SUCCESS(ovr_GetInputState(session, ovrControllerType_Touch, &_touchInputState))) {
                auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
                _touch = std::make_shared<TouchDevice>(*this);
                userInputMapper->registerDevice(_touch);
            }
        }
    });
}

void OculusControllerManager::deactivate() {
    InputPlugin::deactivate();

    // unregister with UserInputMapper
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    if (_touch) {
        userInputMapper->removeDevice(_touch->getDeviceID());
    }
    if (_remote) {
        userInputMapper->removeDevice(_remote->getDeviceID());
    }
}

void OculusControllerManager::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    PerformanceTimer perfTimer("OculusControllerManager::TouchDevice::update");

    checkForConnectedDevices();

    bool updateRemote = false, updateTouch = false;

    ovr::withSession([&](ovrSession session) {
        if (_touch) {
            updateTouch = OVR_SUCCESS(ovr_GetInputState(session, ovrControllerType_Touch, &_touchInputState));
            if (!updateTouch) {
                qCWarning(oculusLog) << "Unable to read Oculus touch input state" << ovr::getError();
            }
        }
        if (_remote) {
            updateRemote = OVR_SUCCESS(ovr_GetInputState(session, ovrControllerType_Remote, &_remoteInputState));
            if (!updateRemote) {
                qCWarning(oculusLog) << "Unable to read Oculus remote input state" << ovr::getError();
            }
        }
    });


    if (_touch && updateTouch) {
        _touch->update(deltaTime, inputCalibrationData);
    }

    if (_remote && updateRemote) {
        _remote->update(deltaTime, inputCalibrationData);
    }
}

void OculusControllerManager::pluginFocusOutEvent() {
    if (_touch) {
        _touch->focusOutEvent();
    }
    if (_remote) {
        _remote->focusOutEvent();
    }
}

void OculusControllerManager::stopHapticPulse(bool leftHand) {
    if (_touch) {
        _touch->stopHapticPulse(leftHand);
    }
}

QStringList OculusControllerManager::getSubdeviceNames() {
    QStringList devices;
    if (_touch) {
        devices << _touch->getName();
    }
    if (_remote) {
        devices << _remote->getName();
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

static const std::vector<std::pair<ovrTouch, StandardButtonChannel>> TOUCH_MAP { {
    { ovrTouch_X, LEFT_PRIMARY_THUMB_TOUCH },
    { ovrTouch_Y, LEFT_SECONDARY_THUMB_TOUCH },
    { ovrTouch_A, RIGHT_PRIMARY_THUMB_TOUCH },
    { ovrTouch_B, RIGHT_SECONDARY_THUMB_TOUCH },
    { ovrTouch_LIndexTrigger, LEFT_PRIMARY_INDEX_TOUCH },
    { ovrTouch_RIndexTrigger, RIGHT_PRIMARY_INDEX_TOUCH },
    { ovrTouch_LThumb, LS_TOUCH },
    { ovrTouch_RThumb, RS_TOUCH },
    { ovrTouch_LThumbUp, LEFT_THUMB_UP },
    { ovrTouch_RThumbUp, RIGHT_THUMB_UP },
    { ovrTouch_LIndexPointing, LEFT_INDEX_POINT },
    { ovrTouch_RIndexPointing, RIGHT_INDEX_POINT },
} };


controller::Input::NamedVector OculusControllerManager::RemoteDevice::getAvailableInputs() const {
    using namespace controller;
    QVector<Input::NamedPair> availableInputs {
        makePair(DU, "DU"),
        makePair(DD, "DD"),
        makePair(DL, "DL"),
        makePair(DR, "DR"),
        makePair(START, "Start"),
        makePair(BACK, "Back"),
    };
    return availableInputs;
}

QString OculusControllerManager::RemoteDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/oculus_remote.json";
    return MAPPING_JSON;
}

void OculusControllerManager::RemoteDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    _buttonPressedMap.clear();
    const auto& inputState = _parent._remoteInputState;
    for (const auto& pair : BUTTON_MAP) {
        if (inputState.Buttons & pair.first) {
            _buttonPressedMap.insert(pair.second);
        }
    }
}

void OculusControllerManager::RemoteDevice::focusOutEvent() {
    _buttonPressedMap.clear();
}

void OculusControllerManager::TouchDevice::update(float deltaTime,
                                                  const controller::InputCalibrationData& inputCalibrationData) {
    _buttonPressedMap.clear();

    int numTrackedControllers = 0;
    quint64 currentTime = usecTimestampNow();
    static const auto REQUIRED_HAND_STATUS = ovrStatus_OrientationTracked | ovrStatus_PositionTracked;
    bool hasInputFocus = ovr::hasInputFocus();
    bool trackControllersInOculusHome = _parent._trackControllersInOculusHome.get();
    auto tracking = ovr::getTrackingState(); // ovr_GetTrackingState(_parent._session, 0, false);
    ovr::for_each_hand([&](ovrHandType hand) {
        ++numTrackedControllers;
        int controller = (hand == ovrHand_Left ? controller::LEFT_HAND : controller::RIGHT_HAND);

        // Disable hand tracking while in Oculus Dash (Dash renders it's own hands)
        if (!hasInputFocus && !trackControllersInOculusHome) {
            _poseStateMap.erase(controller);
            _poseStateMap[controller].valid = false;
            return;
        }

        if (REQUIRED_HAND_STATUS == (tracking.HandStatusFlags[hand] & REQUIRED_HAND_STATUS)) {
            _poseStateMap.erase(controller);
            handlePose(deltaTime, inputCalibrationData, hand, tracking.HandPoses[hand]);
            _lostTracking[controller] = false;
            _lastControllerPose[controller] = tracking.HandPoses[hand];
            return;
        }

        if (_lostTracking[controller]) {
            if (currentTime > _regainTrackingDeadline[controller]) {
                _poseStateMap.erase(controller);
                _poseStateMap[controller].valid = false;
                return;
            }

        } else {
            quint64 deadlineToRegainTracking = currentTime + LOST_TRACKING_DELAY;
            _regainTrackingDeadline[controller] = deadlineToRegainTracking;
            _lostTracking[controller] = true;
        }
        handleRotationForUntrackedHand(inputCalibrationData, hand, tracking.HandPoses[hand]);
    });

    if (ovr::hmdMounted()) {
        handleHeadPose(deltaTime, inputCalibrationData, tracking.HeadPose);
    } else {
        _poseStateMap[controller::HEAD].valid = false;
    }

    using namespace controller;
    // Axes
    const auto& inputState = _parent._touchInputState;
    _axisStateMap[LX].value = inputState.Thumbstick[ovrHand_Left].x;
    _axisStateMap[LY].value = inputState.Thumbstick[ovrHand_Left].y;
    _axisStateMap[LT].value = inputState.IndexTrigger[ovrHand_Left];
    _axisStateMap[LEFT_GRIP].value = inputState.HandTrigger[ovrHand_Left];

    _axisStateMap[RX].value = inputState.Thumbstick[ovrHand_Right].x;
    _axisStateMap[RY].value = inputState.Thumbstick[ovrHand_Right].y;
    _axisStateMap[RT].value = inputState.IndexTrigger[ovrHand_Right];
    _axisStateMap[RIGHT_GRIP].value = inputState.HandTrigger[ovrHand_Right];

    // Buttons
    for (const auto& pair : BUTTON_MAP) {
        if (inputState.Buttons & pair.first) {
            _buttonPressedMap.insert(pair.second);
        }
    }
    // Touches
    for (const auto& pair : TOUCH_MAP) {
        if (inputState.Touches & pair.first) {
            _buttonPressedMap.insert(pair.second);
        }
    }

    // Haptics
    {
        Locker locker(_lock);
        if (_leftHapticDuration > 0.0f) {
            _leftHapticDuration -= deltaTime * 1000.0f; // milliseconds
        } else {
            stopHapticPulse(true);
        }
        if (_rightHapticDuration > 0.0f) {
            _rightHapticDuration -= deltaTime * 1000.0f; // milliseconds
        } else {
            stopHapticPulse(false);
        }
    }
}

void OculusControllerManager::TouchDevice::focusOutEvent() {
    _axisStateMap.clear();
    _buttonPressedMap.clear();
};

void OculusControllerManager::TouchDevice::handlePose(float deltaTime,
                                                      const controller::InputCalibrationData& inputCalibrationData,
                                                      ovrHandType hand, const ovrPoseStatef& handPose) {
    auto poseId = hand == ovrHand_Left ? controller::LEFT_HAND : controller::RIGHT_HAND;
    auto& pose = _poseStateMap[poseId];
    pose = ovr::toControllerPose(hand, handPose);
    // transform into avatar frame
    glm::mat4 controllerToAvatar = glm::inverse(inputCalibrationData.avatarMat) * inputCalibrationData.sensorToWorldMat;
    pose = pose.transform(controllerToAvatar);
}

void OculusControllerManager::TouchDevice::handleHeadPose(float deltaTime,
                                                          const controller::InputCalibrationData& inputCalibrationData,
                                                          const ovrPoseStatef& headPose) {
    glm::mat4 mat = createMatFromQuatAndPos(ovr::toGlm(headPose.ThePose.Orientation),
                                            ovr::toGlm(headPose.ThePose.Position));

    //perform a 180 flip to make the HMD face the +z instead of -z, beacuse the head faces +z
    glm::mat4 matYFlip = mat * Matrices::Y_180;
    controller::Pose pose(extractTranslation(matYFlip),
                          glmExtractRotation(matYFlip),
                          ovr::toGlm(headPose.LinearVelocity), // XXX * matYFlip ?
                          ovr::toGlm(headPose.AngularVelocity));

    glm::mat4 sensorToAvatar = glm::inverse(inputCalibrationData.avatarMat) * inputCalibrationData.sensorToWorldMat;
    glm::mat4 defaultHeadOffset;
    if (inputCalibrationData.hmdAvatarAlignmentType == controller::HmdAvatarAlignmentType::Eyes) {
        // align the eyes of the user with the eyes of the avatar
        defaultHeadOffset = glm::inverse(inputCalibrationData.defaultCenterEyeMat) * inputCalibrationData.defaultHeadMat;
    } else {
        // align the head of the user with the head of the avatar
        defaultHeadOffset = createMatFromQuatAndPos(Quaternions::IDENTITY, -DEFAULT_AVATAR_HEAD_TO_MIDDLE_EYE_OFFSET);
    }

    pose.valid = true;
    _poseStateMap[controller::HEAD] = pose.postTransform(defaultHeadOffset).transform(sensorToAvatar);
}

void OculusControllerManager::TouchDevice::handleRotationForUntrackedHand(const controller::InputCalibrationData& inputCalibrationData,
                                                                          ovrHandType hand, const ovrPoseStatef& handPose) {
    auto poseId = (hand == ovrHand_Left ? controller::LEFT_HAND : controller::RIGHT_HAND);
    auto& pose = _poseStateMap[poseId];
    auto lastHandPose = _lastControllerPose[poseId];
    pose = ovr::toControllerPose(hand, handPose, lastHandPose);
    glm::mat4 controllerToAvatar = glm::inverse(inputCalibrationData.avatarMat) * inputCalibrationData.sensorToWorldMat;
    pose = pose.transform(controllerToAvatar);
}

bool OculusControllerManager::TouchDevice::triggerHapticPulse(float strength, float duration, uint16_t index) {
    if (index > 2) {
        return false;
    }

    controller::Hand hand = (controller::Hand)index;

    Locker locker(_lock);
    bool toReturn = true;
    ovr::withSession([&](ovrSession session) {
        if (hand == controller::BOTH || hand == controller::LEFT) {
            if (strength == 0.0f) {
                _leftHapticStrength = 0.0f;
                _leftHapticDuration = 0.0f;
            } else {
                _leftHapticStrength = (duration > _leftHapticDuration) ? strength : _leftHapticStrength;
                if (ovr_SetControllerVibration(session, ovrControllerType_LTouch, 1.0f, _leftHapticStrength) != ovrSuccess) {
                    toReturn = false;
                }
                _leftHapticDuration = std::max(duration, _leftHapticDuration);
            }
        }
        if (hand == controller::BOTH || hand == controller::RIGHT) {
            if (strength == 0.0f) {
                _rightHapticStrength = 0.0f;
                _rightHapticDuration = 0.0f;
            } else {
                _rightHapticStrength = (duration > _rightHapticDuration) ? strength : _rightHapticStrength;
                if (ovr_SetControllerVibration(session, ovrControllerType_RTouch, 1.0f, _rightHapticStrength) != ovrSuccess) {
                    toReturn = false;
                }
                _rightHapticDuration = std::max(duration, _rightHapticDuration);
            }
        }
    });
    return toReturn;
}

void OculusControllerManager::TouchDevice::stopHapticPulse(bool leftHand) {
    auto handType = (leftHand ? ovrControllerType_LTouch : ovrControllerType_RTouch);
    ovr::withSession([&](ovrSession session) {
        ovr_SetControllerVibration(session, handType, 0.0f, 0.0f);
    });
}

/*@jsdoc
 * <p>The <code>Controller.Hardware.OculusTouch</code> object has properties representing the Oculus Rift. The property values 
 * are integer IDs, uniquely identifying each output. <em>Read-only.</em></p>
 * <p>These outputs can be mapped to actions or functions or <code>Controller.Standard</code> items in a {@link RouteObject} 
 * mapping.</p>
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
controller::Input::NamedVector OculusControllerManager::TouchDevice::getAvailableInputs() const {
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

QString OculusControllerManager::TouchDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/oculus_touch.json";
    return MAPPING_JSON;
}



