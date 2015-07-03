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

//#include <display-plugins\openvr\OpenVrDisplayPlugin.h>
#include <display-plugins\openvr\OpenVrHelpers.h>
#include "UserActivityLogger.h"

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(inputplugins)
Q_LOGGING_CATEGORY(inputplugins, "hifi.inputplugins")


extern vr::IVRSystem *_hmd;
extern vr::TrackedDevicePose_t _trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
extern mat4 _trackedDevicePoseMat4[vr::k_unMaxTrackedDeviceCount];

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
    if (_isInitialized && _isEnabled) {
        
        PerformanceTimer perfTimer("Vive Controllers");

        int trackedControllerCount = 0;
        
        for (vr::TrackedDeviceIndex_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1;
             unTrackedDevice < vr::k_unMaxTrackedDeviceCount && trackedControllerCount < 2; ++unTrackedDevice) {
            
            if (!_hmd->IsTrackedDeviceConnected(unTrackedDevice)) {
                continue;
            }
            
            if(_hmd->GetTrackedDeviceClass(unTrackedDevice) != vr::TrackedDeviceClass_Controller) {
                continue;
            }
            
            trackedControllerCount++;
            
            if(!_trackedDevicePose[unTrackedDevice].bPoseIsValid) {
                continue;
            }
            
            const mat4& mat = _trackedDevicePoseMat4[unTrackedDevice];
                        
            handlePoseEvent(mat, trackedControllerCount - 1);
            
            // handle inputs
            //vr::VRControllerState_t* controllerState;
            //if(_hmd->GetControllerState(unTrackedDevice, controllerState)) {
            //    
            //}

        }
        
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        
        if (trackedControllerCount == 0) {
            if (_deviceID != 0) {
                userInputMapper->removeDevice(_deviceID);
                _deviceID = 0;
                _poseStateMap[makeInput(LEFT_HAND).getChannel()] = UserInputMapper::PoseValue();
                _poseStateMap[makeInput(RIGHT_HAND).getChannel()] = UserInputMapper::PoseValue();
            }
            _trackedControllers = trackedControllerCount;
            return;
        }
        
        if (_trackedControllers == 0 && trackedControllerCount > 0) {
            registerToUserInputMapper(*userInputMapper);
            assignDefaultInputMapping(*userInputMapper);
            UserActivityLogger::getInstance().connectedDevice("spatial_controller", "steamVR");
        }
        
        _trackedControllers = trackedControllerCount;
    }
}

void ViveControllerManager::focusOutEvent() {
    _axisStateMap.clear();
    _buttonPressedMap.clear();
};

void ViveControllerManager::handleAxisEvent(float stickX, float stickY, float trigger, int index) {
//    _axisStateMap[makeInput(AXIS_Y_POS, index).getChannel()] = (stickY > 0.0f) ? stickY : 0.0f;
//    _axisStateMap[makeInput(AXIS_Y_NEG, index).getChannel()] = (stickY < 0.0f) ? -stickY : 0.0f;
//    _axisStateMap[makeInput(AXIS_X_POS, index).getChannel()] = (stickX > 0.0f) ? stickX : 0.0f;
//    _axisStateMap[makeInput(AXIS_X_NEG, index).getChannel()] = (stickX < 0.0f) ? -stickX : 0.0f;
//    _axisStateMap[makeInput(BACK_TRIGGER, index).getChannel()] = trigger;
}

void ViveControllerManager::handleButtonEvent(unsigned int buttons, int index) {
//    if (buttons & BUTTON_0) {
//        _buttonPressedMap.insert(makeInput(BUTTON_0, index).getChannel());
//    }
}

void ViveControllerManager::handlePoseEvent(const mat4& mat, int index) {
    glm::vec4 p = _trackedDevicePoseMat4[vr::k_unTrackedDeviceIndex_Hmd][3];
    glm::vec3 headPos(p.x, p.y, p.z);
    glm::vec3 position = glm::vec3(mat[3][0], mat[3][1], mat[3][2]) - headPos;

    glm::quat rotation = glm::quat_cast(mat);
    //rotation = glm::angleAxis(PI, glm::vec3(0.0f, 1.0f, 0.0f)) * rotation;
    _poseStateMap[makeInput(JointChannel(index)).getChannel()] = UserInputMapper::PoseValue(position, rotation);
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
//        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_0, 0), "Left Start"));
//        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_1, 0), "Left Button 1"));
//        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_2, 0), "Left Button 2"));
//        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_3, 0), "Left Button 3"));
//        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_4, 0), "Left Button 4"));
//        
//        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_FWD, 0), "L1"));
//        availableInputs.append(UserInputMapper::InputPair(makeInput(BACK_TRIGGER, 0), "L2"));
//        
//        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_Y_POS, 0), "Left Stick Up"));
//        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_Y_NEG, 0), "Left Stick Down"));
//        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_X_POS, 0), "Left Stick Right"));
//        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_X_NEG, 0), "Left Stick Left"));
//        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_TRIGGER, 0), "Left Trigger Press"));
//        
//        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_0, 1), "Right Start"));
//        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_1, 1), "Right Button 1"));
//        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_2, 1), "Right Button 2"));
//        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_3, 1), "Right Button 3"));
//        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_4, 1), "Right Button 4"));
//        
//        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_FWD, 1), "R1"));
//        availableInputs.append(UserInputMapper::InputPair(makeInput(BACK_TRIGGER, 1), "R2"));
//        
//        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_Y_POS, 1), "Right Stick Up"));
//        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_Y_NEG, 1), "Right Stick Down"));
//        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_X_POS, 1), "Right Stick Right"));
//        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_X_NEG, 1), "Right Stick Left"));
//        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_TRIGGER, 1), "Right Trigger Press"));
        
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
    const float JOYSTICK_YAW_SPEED = 0.5f;
    const float JOYSTICK_PITCH_SPEED = 0.25f;
    const float BUTTON_MOVE_SPEED = 1.0f;
    const float BOOM_SPEED = 0.1f;
    
//    // Left Joystick: Movement, strafing
//    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_FORWARD, makeInput(AXIS_Y_POS, 0), JOYSTICK_MOVE_SPEED);
//    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_BACKWARD, makeInput(AXIS_Y_NEG, 0), JOYSTICK_MOVE_SPEED);
//    mapper.addInputChannel(UserInputMapper::LATERAL_RIGHT, makeInput(AXIS_X_POS, 0), JOYSTICK_MOVE_SPEED);
//    mapper.addInputChannel(UserInputMapper::LATERAL_LEFT, makeInput(AXIS_X_NEG, 0), JOYSTICK_MOVE_SPEED);
//    
//    // Right Joystick: Camera orientation
//    mapper.addInputChannel(UserInputMapper::YAW_RIGHT, makeInput(AXIS_X_POS, 1), JOYSTICK_YAW_SPEED);
//    mapper.addInputChannel(UserInputMapper::YAW_LEFT, makeInput(AXIS_X_NEG, 1), JOYSTICK_YAW_SPEED);
//    mapper.addInputChannel(UserInputMapper::PITCH_UP, makeInput(AXIS_Y_POS, 1), JOYSTICK_PITCH_SPEED);
//    mapper.addInputChannel(UserInputMapper::PITCH_DOWN, makeInput(AXIS_Y_NEG, 1), JOYSTICK_PITCH_SPEED);
//    
//    // Buttons
//    mapper.addInputChannel(UserInputMapper::BOOM_IN, makeInput(BUTTON_3, 0), BOOM_SPEED);
//    mapper.addInputChannel(UserInputMapper::BOOM_OUT, makeInput(BUTTON_1, 0), BOOM_SPEED);
//    
//    mapper.addInputChannel(UserInputMapper::VERTICAL_UP, makeInput(BUTTON_3, 1), BUTTON_MOVE_SPEED);
//    mapper.addInputChannel(UserInputMapper::VERTICAL_DOWN, makeInput(BUTTON_1, 1), BUTTON_MOVE_SPEED);
//    
//    mapper.addInputChannel(UserInputMapper::SHIFT, makeInput(BUTTON_2, 0));
//    mapper.addInputChannel(UserInputMapper::SHIFT, makeInput(BUTTON_2, 1));
//    
//    mapper.addInputChannel(UserInputMapper::ACTION1, makeInput(BUTTON_4, 0));
//    mapper.addInputChannel(UserInputMapper::ACTION2, makeInput(BUTTON_4, 1));
    
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
    return UserInputMapper::Input();
//    return UserInputMapper::Input(_deviceID, button | (index == 0 ? LEFT_MASK : RIGHT_MASK), UserInputMapper::ChannelType::BUTTON);
}

UserInputMapper::Input ViveControllerManager::makeInput(ViveControllerManager::JoystickAxisChannel axis, int index) {
    return UserInputMapper::Input();
//    return UserInputMapper::Input(_deviceID, axis | (index == 0 ? LEFT_MASK : RIGHT_MASK), UserInputMapper::ChannelType::AXIS);
}

UserInputMapper::Input ViveControllerManager::makeInput(JointChannel joint) {
    return UserInputMapper::Input(_deviceID, joint, UserInputMapper::ChannelType::POSE);
}
