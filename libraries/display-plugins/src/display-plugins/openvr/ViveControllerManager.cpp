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

#include "Application.h"
#include <avatar/AvatarManager.h>
#include "OpenVrDisplayPlugin.h"
#include "OpenVrHelpers.h"
#include "UserActivityLogger.h"

extern vr::IVRSystem *_hmd{ nullptr };
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
    _prevPalms[0] = nullptr;
    _prevPalms[1] = nullptr;
}

ViveControllerManager::~ViveControllerManager() {

}

void ViveControllerManager::activate() {
    if (!_hmd) {
        vr::HmdError eError = vr::HmdError_None;
        _hmd = vr::VR_Init(&eError);
        Q_ASSERT(eError == vr::HmdError_None);
        Q_ASSERT(_hmd);
    }
    _isInitialized = true;
}

void ViveControllerManager::update() {
    Hand* hand = DependencyManager::get<AvatarManager>()->getMyAvatar()->getHand();
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
            
            const Matrix4 & mat = _trackedDevicePoseMat4[unTrackedDevice];
            
            PalmData* palm;
            bool foundHand = false;
            // FIXME: this shouldn't use SixenseID
            for (size_t j = 0; j < hand->getNumPalms(); j++) {
                if (hand->getPalms()[j].getSixenseID() == unTrackedDevice) {
                    palm = &(hand->getPalms()[j]);
                    _prevPalms[trackedControllerCount - 1] = palm;
                    foundHand = true;
                }
            }
            if (!foundHand) {
                PalmData newPalm(hand);
                hand->getPalms().push_back(newPalm);
                palm = &(hand->getPalms()[hand->getNumPalms() - 1]);
                palm->setSixenseID(unTrackedDevice);
                _prevPalms[trackedControllerCount - 1] = palm;
                qCDebug(interfaceapp, "Found new Vive hand controller, ID %i", unTrackedDevice);
            }
            
            palm->setActive(true);
            
            // handle inputs
            vr::VRControllerState_t* controllerState;
            if(vr::GetControllerState(unTrackedDevice, controllerState)) {
                
            }
            
            // set position and rotation
//          m.m[0][0], m.m[1][0], m.m[2][0], 0.0,
//          m.m[0][1], m.m[1][1], m.m[2][1], 0.0,
//          m.m[0][2], m.m[1][2], m.m[2][2], 0.0,
//          m.m[0][3], m.m[1][3], m.m[2][3], 1.0f);
            glm::vec3 position(_trackedDevicePoseMat4[3][0], _trackedDevicePoseMat4[3][1], _trackedDevicePoseMat4[3][2]);
//           position *= METERS_PER_MILLIMETER;

//           // Transform the measured position into body frame.
//           glm::vec3 neck = _neckBase;
//           // Zeroing y component of the "neck" effectively raises the measured position a little bit.
//           neck.y = 0.0f;
//           position = _orbRotation * (position - neck);

//           //  Rotation of Palm
            float w = sqrt(1.0f + _trackedDevicePoseMat4[0][0] + _trackedDevicePoseMat4[1][1] + _trackedDevicePoseMat4[2][2]) / 2.0f;
            float x = (_trackedDevicePoseMat4[2][1] - _trackedDevicePoseMat4[1][2])/(4.0f * w);
            float y = (_trackedDevicePoseMat4[0][2] - _trackedDevicePoseMat4[2][0])/(4.0f * w);
            float z = (_trackedDevicePoseMat4[1][0] - _trackedDevicePoseMat4[0][1])/(4.0f * w);
            glm::quat rotation(w, x, y, z);
            rotation = glm::angleAxis(PI, glm::vec3(0.0f, 1.0f, 0.0f)) * rotation;

//           //  Compute current velocity from position change
//           glm::vec3 rawVelocity;
//           if (deltaTime > 0.0f) {
//               rawVelocity = (position - palm->getRawPosition()) / deltaTime;
//           } else {
//               rawVelocity = glm::vec3(0.0f);
//           }
//           palm->setRawVelocity(rawVelocity);   //  meters/sec
//           
//           // adjustment for hydra controllers fit into hands
//           float sign = (i == 0) ? -1.0f : 1.0f;
//           rotation *= glm::angleAxis(sign * PI/4.0f, glm::vec3(0.0f, 0.0f, 1.0f));
//           
//           //  Angular Velocity of Palm
//           glm::quat deltaRotation = rotation * glm::inverse(palm->getRawRotation());
//           glm::vec3 angularVelocity(0.0f);
//           float rotationAngle = glm::angle(deltaRotation);
//           if ((rotationAngle > EPSILON) && (deltaTime > 0.0f)) {
//               angularVelocity = glm::normalize(glm::axis(deltaRotation));
//               angularVelocity *= (rotationAngle / deltaTime);
//               palm->setRawAngularVelocity(angularVelocity);
//           } else {
//               palm->setRawAngularVelocity(glm::vec3(0.0f));
//           }
//           
//           if (_lowVelocityFilter) {
//               //  Use a velocity sensitive filter to damp small motions and preserve large ones with
//               //  no latency.
//               float velocityFilter = glm::clamp(1.0f - glm::length(rawVelocity), 0.0f, 1.0f);
//               position = palm->getRawPosition() * velocityFilter + position * (1.0f - velocityFilter);
//               rotation = safeMix(palm->getRawRotation(), rotation, 1.0f - velocityFilter);
//               palm->setRawPosition(position);
//               palm->setRawRotation(rotation);
//           } else {
               palm->setRawPosition(position);
               palm->setRawRotation(rotation);
//           }
//           
//           // Store the one fingertip in the palm structure so we can track velocity
//           const float FINGER_LENGTH = 0.3f;   //  meters
//           const glm::vec3 FINGER_VECTOR(0.0f, 0.0f, FINGER_LENGTH);
//           const glm::vec3 newTipPosition = position + rotation * FINGER_VECTOR;
//           glm::vec3 oldTipPosition = palm->getTipRawPosition();
//           if (deltaTime > 0.0f) {
//               palm->setTipVelocity((newTipPosition - oldTipPosition) / deltaTime);
//           } else {
//               palm->setTipVelocity(glm::vec3(0.0f));
//           }
//           palm->setTipPosition(newTipPosition);
        }
        
        if (trackedControllerCount == 0) {
            if (_deviceID != 0) {
                Application::getUserInputMapper()->removeDevice(_deviceID);
                _deviceID = 0;
                if (_prevPalms[0]) {
                    _prevPalms[0]->setActive(false);
                }
                if (_prevPalms[1]) {
                    _prevPalms[1]->setActive(false);
                }
            }
            _trackedControllers = trackedControllerCount;
            return;
        }
        
        if (_trackedControllers == 0 && trackedControllerCount > 0) {
            registerToUserInputMapper(*Application::getUserInputMapper());
            assignDefaultInputMapping(*Application::getUserInputMapper());
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

void ViveControllerManager::registerToUserInputMapper(UserInputMapper& mapper) {
    // Grab the current free device ID
    _deviceID = mapper.getFreeDeviceID();
    
    auto proxy = UserInputMapper::DeviceProxy::Pointer(new UserInputMapper::DeviceProxy("SteamVR Controller"));
    proxy->getButton = [this] (const UserInputMapper::Input& input, int timestamp) -> bool { return this->getButton(input.getChannel()); };
    proxy->getAxis = [this] (const UserInputMapper::Input& input, int timestamp) -> float { return this->getAxis(input.getChannel()); };
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

UserInputMapper::Input ViveControllerManager::makeInput(unsigned int button, int index) {
    return UserInputMapper::Input(0);
//    return UserInputMapper::Input(_deviceID, button | (index == 0 ? LEFT_MASK : RIGHT_MASK), UserInputMapper::ChannelType::BUTTON);
}

UserInputMapper::Input ViveControllerManager::makeInput(ViveControllerManager::JoystickAxisChannel axis, int index) {
    return UserInputMapper::Input(0);
//    return UserInputMapper::Input(_deviceID, axis | (index == 0 ? LEFT_MASK : RIGHT_MASK), UserInputMapper::ChannelType::AXIS);
}
