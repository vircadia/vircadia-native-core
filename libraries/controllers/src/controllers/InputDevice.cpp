//
//  InputDevice.cpp
//  input-plugins/src/input-plugins
//
//  Created by Sam Gondelman on 7/15/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "InputDevice.h"

bool InputDevice::_lowVelocityFilter = false;

const float DEFAULT_HAND_RETICLE_MOVE_SPEED = 37.5f;
float InputDevice::reticleMoveSpeed = DEFAULT_HAND_RETICLE_MOVE_SPEED;

//Constants for getCursorPixelRangeMultiplier()
const float MIN_PIXEL_RANGE_MULT = 0.4f;
const float MAX_PIXEL_RANGE_MULT = 2.0f;
const float RANGE_MULT = (MAX_PIXEL_RANGE_MULT - MIN_PIXEL_RANGE_MULT) * 0.01f;

//Returns a multiplier to be applied to the cursor range for the controllers
float InputDevice::getCursorPixelRangeMult() {
    //scales (0,100) to (MINIMUM_PIXEL_RANGE_MULT, MAXIMUM_PIXEL_RANGE_MULT)
    return InputDevice::reticleMoveSpeed * RANGE_MULT + MIN_PIXEL_RANGE_MULT;
}

float InputDevice::getButton(int channel) const {
    if (!_buttonPressedMap.empty()) {
        if (_buttonPressedMap.find(channel) != _buttonPressedMap.end()) {
            return 1.0f;
        }
        else {
            return 0.0f;
        }
    }
    return 0.0f;
}

float InputDevice::getAxis(int channel) const {
    auto axis = _axisStateMap.find(channel);
    if (axis != _axisStateMap.end()) {
        return (*axis).second;
    }
    else {
        return 0.0f;
    }
}

UserInputMapper::PoseValue InputDevice::getPose(int channel) const {
    auto pose = _poseStateMap.find(channel);
    if (pose != _poseStateMap.end()) {
        return (*pose).second;
    }
    else {
        return UserInputMapper::PoseValue();
    }
}

UserInputMapper::Input InputDevice::makeInput(controller::StandardButtonChannel button) {
    return UserInputMapper::Input(_deviceID, button, UserInputMapper::ChannelType::BUTTON);
}

UserInputMapper::Input InputDevice::makeInput(controller::StandardAxisChannel axis) {
    return UserInputMapper::Input(_deviceID, axis, UserInputMapper::ChannelType::AXIS);
}

UserInputMapper::Input InputDevice::makeInput(controller::StandardPoseChannel pose) {
    return UserInputMapper::Input(_deviceID, pose, UserInputMapper::ChannelType::POSE);
}

UserInputMapper::InputPair InputDevice::makePair(controller::StandardButtonChannel button, const QString& name) {
    return UserInputMapper::InputPair(makeInput(button), name);
}

UserInputMapper::InputPair InputDevice::makePair(controller::StandardAxisChannel axis, const QString& name) {
    return UserInputMapper::InputPair(makeInput(axis), name);
}

UserInputMapper::InputPair InputDevice::makePair(controller::StandardPoseChannel pose, const QString& name) {
    return UserInputMapper::InputPair(makeInput(pose), name);
}

