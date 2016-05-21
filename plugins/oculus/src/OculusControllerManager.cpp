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

#include <plugins/PluginContainer.h>
#include <controllers/UserInputMapper.h>
#include <controllers/StandardControls.h>

#include <PerfStat.h>
#include <PathUtils.h>

#include "OculusHelpers.h"

Q_DECLARE_LOGGING_CATEGORY(oculus)


static const QString MENU_PARENT = "Avatar";
static const QString MENU_NAME = "Oculus Touch Controllers";
static const QString MENU_PATH = MENU_PARENT + ">" + MENU_NAME;

const QString OculusControllerManager::NAME = "Oculus";

bool OculusControllerManager::isSupported() const {
    return oculusAvailable();
}

bool OculusControllerManager::activate() {
    InputPlugin::activate();
    if (!_session) {
        _session = acquireOculusSession();
    }
    Q_ASSERT(_session);

    // register with UserInputMapper
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();

    if (OVR_SUCCESS(ovr_GetInputState(_session, ovrControllerType_Remote, &_inputState))) {
        _remote = std::make_shared<RemoteDevice>(*this);
        userInputMapper->registerDevice(_remote);
    }

    if (OVR_SUCCESS(ovr_GetInputState(_session, ovrControllerType_Touch, &_inputState))) {
        _touch = std::make_shared<TouchDevice>(*this);
        userInputMapper->registerDevice(_touch);
    }

    return true;
}

void OculusControllerManager::deactivate() {
    InputPlugin::deactivate();

    if (_session) {
        releaseOculusSession();
        _session = nullptr;
    }

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

    if (_touch) {
        if (OVR_SUCCESS(ovr_GetInputState(_session, ovrControllerType_Touch, &_inputState))) {
            _touch->update(deltaTime, inputCalibrationData);
        } else {
            qCWarning(oculus) << "Unable to read Oculus touch input state";
        }
    }

    if (_remote) {
        if (OVR_SUCCESS(ovr_GetInputState(_session, ovrControllerType_Remote, &_inputState))) {
            _remote->update(deltaTime, inputCalibrationData);
        } else {
            qCWarning(oculus) << "Unable to read Oculus remote input state";
        }
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
    { ovrButton_LShoulder, LB },
    { ovrButton_RShoulder, RB },
} };

static const std::vector<std::pair<ovrTouch, StandardButtonChannel>> TOUCH_MAP { {
    { ovrTouch_X, LEFT_SECONDARY_THUMB_TOUCH },
    { ovrTouch_Y, LEFT_SECONDARY_THUMB_TOUCH },
    { ovrTouch_A, RIGHT_SECONDARY_THUMB_TOUCH },
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
    const auto& inputState = _parent._inputState;
    for (const auto& pair : BUTTON_MAP) {
        if (inputState.Buttons & pair.first) {
            _buttonPressedMap.insert(pair.second);
        }
    }
}

void OculusControllerManager::RemoteDevice::focusOutEvent() {
    _buttonPressedMap.clear();
}

void OculusControllerManager::TouchDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    _poseStateMap.clear();
    _buttonPressedMap.clear();

    int numTrackedControllers = 0;
    static const auto REQUIRED_HAND_STATUS = ovrStatus_OrientationTracked & ovrStatus_PositionTracked;
    auto tracking = ovr_GetTrackingState(_parent._session, 0, false);
    ovr_for_each_hand([&](ovrHandType hand) {
        ++numTrackedControllers;
        if (REQUIRED_HAND_STATUS == (tracking.HandStatusFlags[hand] & REQUIRED_HAND_STATUS)) {
            handlePose(deltaTime, inputCalibrationData, hand, tracking.HandPoses[hand]);
        }
    });
    using namespace controller;
    // Axes
    const auto& inputState = _parent._inputState;
    _axisStateMap[LX] = inputState.Thumbstick[ovrHand_Left].x;
    _axisStateMap[LY] = inputState.Thumbstick[ovrHand_Left].y;
    _axisStateMap[LT] = inputState.IndexTrigger[ovrHand_Left];
    _axisStateMap[LG] = inputState.HandTrigger[ovrHand_Left];

    _axisStateMap[RX] = inputState.Thumbstick[ovrHand_Right].x;
    _axisStateMap[RY] = inputState.Thumbstick[ovrHand_Right].y;
    _axisStateMap[RT] = inputState.IndexTrigger[ovrHand_Right];
    _axisStateMap[RG] = inputState.HandTrigger[ovrHand_Right];

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
}

void OculusControllerManager::TouchDevice::focusOutEvent() {
    _axisStateMap.clear();
    _buttonPressedMap.clear();
};

void OculusControllerManager::TouchDevice::handlePose(float deltaTime, 
    const controller::InputCalibrationData& inputCalibrationData, ovrHandType hand, 
    const ovrPoseStatef& handPose) {
    auto poseId = hand == ovrHand_Left ? controller::LEFT_HAND : controller::RIGHT_HAND;
    auto& pose = _poseStateMap[poseId];
    pose.translation = toGlm(handPose.ThePose.Position);
    pose.rotation = toGlm(handPose.ThePose.Orientation);
    pose.angularVelocity = toGlm(handPose.AngularVelocity);
    pose.velocity = toGlm(handPose.LinearVelocity);
}

controller::Input::NamedVector OculusControllerManager::TouchDevice::getAvailableInputs() const {
    using namespace controller;
    QVector<Input::NamedPair> availableInputs{
        // Trackpad analogs
        makePair(LX, "LX"),
        makePair(LY, "LY"),
        makePair(RX, "RX"),
        makePair(RY, "RY"),
        // trigger analogs
        makePair(LT, "LT"),
        makePair(RT, "RT"),

        makePair(LB, "LB"),
        makePair(RB, "RB"),

        makePair(LS, "LS"),
        makePair(RS, "RS"),
        makePair(LEFT_HAND, "LeftHand"),
        makePair(RIGHT_HAND, "RightHand"),

        makePair(LEFT_PRIMARY_THUMB, "LeftPrimaryThumb"),
        makePair(LEFT_SECONDARY_THUMB, "LeftSecondaryThumb"),
        makePair(RIGHT_PRIMARY_THUMB, "RightPrimaryThumb"),
        makePair(RIGHT_SECONDARY_THUMB, "RightSecondaryThumb"),
    };
    return availableInputs;
}

QString OculusControllerManager::TouchDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/oculus_touch.json";
    return MAPPING_JSON;
}



