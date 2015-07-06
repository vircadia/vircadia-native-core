//
//  ViveControllerManager.cpp
//  interface/src/devices
//
//  Created by Sam Gondelman on 6/29/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ViveControllerManager.h"

#include <PerfStat.h>

#include <display-plugins\openvr\OpenVrHelpers.h>
#include "NumericalConstants.h"
#include "UserActivityLogger.h"

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(inputplugins)
Q_LOGGING_CATEGORY(inputplugins, "hifi.inputplugins")

extern vr::IVRSystem *_hmd;
extern vr::TrackedDevicePose_t _trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
extern mat4 _trackedDevicePoseMat4[vr::k_unMaxTrackedDeviceCount];

const unsigned int LEFT_MASK = 0U;
const unsigned int RIGHT_MASK = 1U;

const uint64_t VR_BUTTON_A = 1U << 1;
const uint64_t VR_GRIP_BUTTON = 1U << 2;
const uint64_t VR_TRACKPAD_BUTTON = 1ULL << 32;

const unsigned int BUTTON_A = 1U << 1;
const unsigned int GRIP_BUTTON = 1U << 2;
const unsigned int TRACKPAD_BUTTON = 1U << 3;

ViveControllerManager& ViveControllerManager::getInstance() {
    static ViveControllerManager sharedInstance;
    return sharedInstance;
}

ViveControllerManager::ViveControllerManager() :
    _isInitialized(false),
    _isEnabled(true),
    _trackedControllers(0)
{
    
}

ViveControllerManager::~ViveControllerManager() {

}

void ViveControllerManager::activate() {
    if (!_hmd) {
        vr::HmdError eError = vr::HmdError_None;
        _hmd = vr::VR_Init(&eError);
        Q_ASSERT(eError == vr::HmdError_None);
    }
    Q_ASSERT(_hmd);
    _isInitialized = true;
}

void ViveControllerManager::update() {
    if (!_hmd) {
        return;
    }
    _buttonPressedMap.clear();
    if (_isInitialized && _isEnabled) {
        
        PerformanceTimer perfTimer("Vive Controllers");

        int numTrackedControllers = 0;
        
        for (vr::TrackedDeviceIndex_t device = vr::k_unTrackedDeviceIndex_Hmd + 1;
             device < vr::k_unMaxTrackedDeviceCount && numTrackedControllers < 2; ++device) {
            
            if (!_hmd->IsTrackedDeviceConnected(device)) {
                continue;
            }
            
            if(_hmd->GetTrackedDeviceClass(device) != vr::TrackedDeviceClass_Controller) {
                continue;
            }

            if (!_trackedDevicePose[device].bPoseIsValid) {
                continue;
            }
            
            numTrackedControllers++;
            
            const mat4& mat = _trackedDevicePoseMat4[device];
                        
            handlePoseEvent(mat, numTrackedControllers - 1);
            
            // handle inputs
            vr::VRControllerState_t controllerState = vr::VRControllerState_t();
            if (_hmd->GetControllerState(device, &controllerState)) {
                handleButtonEvent(controllerState.ulButtonPressed, numTrackedControllers - 1);
                for (int i = 0; i < 5; i++) {
                    handleAxisEvent(Axis(i), controllerState.rAxis[i].x, controllerState.rAxis[i].y, numTrackedControllers - 1);
                }
            }
        }
        
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        
        if (numTrackedControllers == 0) {
            if (_deviceID != 0) {
                userInputMapper->removeDevice(_deviceID);
                _deviceID = 0;
                _poseStateMap[makeInput(LEFT_HAND).getChannel()] = UserInputMapper::PoseValue();
                _poseStateMap[makeInput(RIGHT_HAND).getChannel()] = UserInputMapper::PoseValue();
            }
            _trackedControllers = numTrackedControllers;
            return;
        }
        
        if (_trackedControllers == 0 && numTrackedControllers > 0) {
            registerToUserInputMapper(*userInputMapper);
            assignDefaultInputMapping(*userInputMapper);
            UserActivityLogger::getInstance().connectedDevice("spatial_controller", "steamVR");
        }
        
        _trackedControllers = numTrackedControllers;
    }
}

void ViveControllerManager::focusOutEvent() {
    _axisStateMap.clear();
    _buttonPressedMap.clear();
};

void ViveControllerManager::handleAxisEvent(Axis axis, float x, float y, int index) {
    if (axis == TRACKPAD_AXIS) {
        _axisStateMap[makeInput(AXIS_Y_POS, index).getChannel()] = (y > 0.0f) ? y : 0.0f;
        _axisStateMap[makeInput(AXIS_Y_NEG, index).getChannel()] = (y < 0.0f) ? -y : 0.0f;
        _axisStateMap[makeInput(AXIS_X_POS, index).getChannel()] = (x > 0.0f) ? x : 0.0f;
        _axisStateMap[makeInput(AXIS_X_NEG, index).getChannel()] = (x < 0.0f) ? -x : 0.0f;
    } else if (axis == TRIGGER_AXIS) {
        _axisStateMap[makeInput(BACK_TRIGGER, index).getChannel()] = x;
    }
}

void ViveControllerManager::handleButtonEvent(uint64_t buttons, int index) {
    if (buttons & VR_BUTTON_A) {
        _buttonPressedMap.insert(makeInput(BUTTON_A, index).getChannel());
    }
    if (buttons & VR_GRIP_BUTTON) {
        _buttonPressedMap.insert(makeInput(GRIP_BUTTON, index).getChannel());
    }
    if (buttons & VR_TRACKPAD_BUTTON) {
        _buttonPressedMap.insert(makeInput(TRACKPAD_BUTTON, index).getChannel());
    }
}

void ViveControllerManager::handlePoseEvent(const mat4& mat, int index) {
    glm::vec4 p = _trackedDevicePoseMat4[vr::k_unTrackedDeviceIndex_Hmd][3];
    glm::vec3 headPos(p.x, p.y, p.z);
    glm::vec3 position = glm::vec3(mat[3][0], mat[3][1], mat[3][2]) - headPos;

    glm::quat rotation = glm::quat_cast(mat);

    // Flip the rotation appropriately for each hand
    if (index == LEFT_HAND) {
        rotation = rotation * glm::angleAxis(PI, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::angleAxis(PI_OVER_TWO, glm::vec3(0.0f, 0.0f, 1.0f));
    } else if (index == RIGHT_HAND) {
        rotation = rotation * glm::angleAxis(PI, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::angleAxis(PI + PI_OVER_TWO, glm::vec3(0.0f, 0.0f, 1.0f));
    }

    _poseStateMap[makeInput(JointChannel(index)).getChannel()] = UserInputMapper::PoseValue(position, - rotation);
}

void ViveControllerManager::registerToUserInputMapper(UserInputMapper& mapper) {
    // Grab the current free device ID
    _deviceID = mapper.getFreeDeviceID();
    
    auto proxy = UserInputMapper::DeviceProxy::Pointer(new UserInputMapper::DeviceProxy("SteamVR Controller"));
    proxy->getButton = [this] (const UserInputMapper::Input& input, int timestamp) -> bool { return this->getButton(input.getChannel()); };
    proxy->getAxis = [this] (const UserInputMapper::Input& input, int timestamp) -> float { return this->getAxis(input.getChannel()); };
    proxy->getPose = [this](const UserInputMapper::Input& input, int timestamp) -> UserInputMapper::PoseValue { return this->getPose(input.getChannel()); };
    proxy->getAvailabeInputs = [this] () -> QVector<UserInputMapper::InputPair> {
        QVector<UserInputMapper::InputPair> availableInputs;
        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_A, 0), "Left Button A"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(GRIP_BUTTON, 0), "Left Grip Button"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(TRACKPAD_BUTTON, 0), "Left Trackpad Button"));

        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_Y_POS, 0), "Left Trackpad Up"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_Y_NEG, 0), "Left Trackpad Down"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_X_POS, 0), "Left Trackpad Right"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_X_NEG, 0), "Left Trackpad Left"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(BACK_TRIGGER, 0), "Left Back Trigger"));

        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_A, 1), "Right Button A"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(GRIP_BUTTON, 1), "Right Grip Button"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(TRACKPAD_BUTTON, 1), "Right Trackpad Button"));

        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_Y_POS, 1), "Right Trackpad Up"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_Y_NEG, 1), "Right Trackpad Down"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_X_POS, 1), "Right Trackpad Right"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_X_NEG, 1), "Right Trackpad Left"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(BACK_TRIGGER, 1), "Right Back Trigger"));
        
        return availableInputs;
    };
    proxy->resetDeviceBindings = [this, &mapper] () -> bool {
        mapper.removeAllInputChannelsForDevice(_deviceID);
        this->assignDefaultInputMapping(mapper);
        return true;
    };
    mapper.registerDevice(_deviceID, proxy);
}

void ViveControllerManager::assignDefaultInputMapping(UserInputMapper& mapper) {
    const float JOYSTICK_MOVE_SPEED = 1.0f;
    const float BOOM_SPEED = 0.1f;
    
    // Left Trackpad: Movement, strafing
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_FORWARD, makeInput(AXIS_Y_POS, 0), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_BACKWARD, makeInput(AXIS_Y_NEG, 0), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LATERAL_RIGHT, makeInput(AXIS_X_POS, 0), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LATERAL_LEFT, makeInput(AXIS_X_NEG, 0), JOYSTICK_MOVE_SPEED);

    // Right Trackpad: Vertical movement, zooming
    mapper.addInputChannel(UserInputMapper::VERTICAL_UP, makeInput(AXIS_Y_POS, 1), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::VERTICAL_DOWN, makeInput(AXIS_Y_NEG, 1), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::BOOM_IN, makeInput(AXIS_X_POS, 1), BOOM_SPEED);
    mapper.addInputChannel(UserInputMapper::BOOM_OUT, makeInput(AXIS_X_NEG, 1), BOOM_SPEED);
    
    // Buttons
    mapper.addInputChannel(UserInputMapper::SHIFT, makeInput(GRIP_BUTTON, 0));
    mapper.addInputChannel(UserInputMapper::SHIFT, makeInput(GRIP_BUTTON, 1));
    
    mapper.addInputChannel(UserInputMapper::ACTION1, makeInput(BUTTON_A, 0));
    mapper.addInputChannel(UserInputMapper::ACTION2, makeInput(BUTTON_A, 1));
    
    // Hands
    mapper.addInputChannel(UserInputMapper::LEFT_HAND, makeInput(LEFT_HAND));
    mapper.addInputChannel(UserInputMapper::RIGHT_HAND, makeInput(RIGHT_HAND));
}

float ViveControllerManager::getButton(int channel) const {
    if (!_buttonPressedMap.empty()) {
        if (_buttonPressedMap.find(channel) != _buttonPressedMap.end()) {
            return 1.0f;
        } else {
            return 0.0f;
        }
    }
    return 0.0f;
}

float ViveControllerManager::getAxis(int channel) const {
    auto axis = _axisStateMap.find(channel);
    if (axis != _axisStateMap.end()) {
        return (*axis).second;
    } else {
        return 0.0f;
    }
}

UserInputMapper::PoseValue ViveControllerManager::getPose(int channel) const {
    auto pose = _poseStateMap.find(channel);
    if (pose != _poseStateMap.end()) {
        return (*pose).second;
    } else {
        return UserInputMapper::PoseValue();
    }
}

UserInputMapper::Input ViveControllerManager::makeInput(unsigned int button, int index) {
    return UserInputMapper::Input(_deviceID, button | (index == 0 ? LEFT_MASK : RIGHT_MASK), UserInputMapper::ChannelType::BUTTON);
}

UserInputMapper::Input ViveControllerManager::makeInput(ViveControllerManager::JoystickAxisChannel axis, int index) {
    return UserInputMapper::Input(_deviceID, axis | (index == 0 ? LEFT_MASK : RIGHT_MASK), UserInputMapper::ChannelType::AXIS);
}

UserInputMapper::Input ViveControllerManager::makeInput(JointChannel joint) {
    return UserInputMapper::Input(_deviceID, joint, UserInputMapper::ChannelType::POSE);
}
