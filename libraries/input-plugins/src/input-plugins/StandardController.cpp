//
//  StandardController.cpp
//  input-plugins/src/input-plugins
//
//  Created by Brad Hefta-Gaub on 2015-10-11.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <limits>

#include <glm/glm.hpp>

#include "StandardController.h"

const float CONTROLLER_THRESHOLD = 0.3f;

const float MAX_AXIS = 32768.0f;

StandardController::~StandardController() {
}

void StandardController::update(float deltaTime, bool jointsCaptured) {
    for (auto axisState : _axisStateMap) {
        if (fabsf(axisState.second) < CONTROLLER_THRESHOLD) {
            _axisStateMap[axisState.first] = 0.0f;
        }
    }
}

void StandardController::focusOutEvent() {
    _axisStateMap.clear();
    _buttonPressedMap.clear();
};

void StandardController::registerToUserInputMapper(UserInputMapper& mapper) {
    // Grab the current free device ID
    _deviceID = mapper.getStandardDeviceID();
    
    auto proxy = std::make_shared<UserInputMapper::DeviceProxy>(_name);
    proxy->getButton = [this] (const UserInputMapper::Input& input, int timestamp) -> bool { return this->getButton(input.getChannel()); };
    proxy->getAxis = [this] (const UserInputMapper::Input& input, int timestamp) -> float { return this->getAxis(input.getChannel()); };
    proxy->getAvailabeInputs = [this] () -> QVector<UserInputMapper::InputPair> {
        QVector<UserInputMapper::InputPair> availableInputs;
        availableInputs.append(UserInputMapper::InputPair(makeInput(STANDARD_CONTROLLER_BUTTON_A), "Bottom Button"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(STANDARD_CONTROLLER_BUTTON_B), "Right Button"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(STANDARD_CONTROLLER_BUTTON_X), "Left Button"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(STANDARD_CONTROLLER_BUTTON_Y), "Top Button"));
        
        availableInputs.append(UserInputMapper::InputPair(makeInput(STANDARD_CONTROLLER_BUTTON_DPAD_UP), "DPad Up"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(STANDARD_CONTROLLER_BUTTON_DPAD_DOWN), "DPad Down"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(STANDARD_CONTROLLER_BUTTON_DPAD_LEFT), "DPad Left"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(STANDARD_CONTROLLER_BUTTON_DPAD_RIGHT), "DPad Right"));
        
        availableInputs.append(UserInputMapper::InputPair(makeInput(STANDARD_CONTROLLER_BUTTON_LEFTSHOULDER), "L1"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(STANDARD_CONTROLLER_BUTTON_RIGHTSHOULDER), "R1"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(RIGHT_SHOULDER), "L2"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(LEFT_SHOULDER), "R2"));
        
        availableInputs.append(UserInputMapper::InputPair(makeInput(LEFT_AXIS_Y_NEG), "Left Stick Up"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(LEFT_AXIS_Y_POS), "Left Stick Down"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(LEFT_AXIS_X_POS), "Left Stick Right"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(LEFT_AXIS_X_NEG), "Left Stick Left"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(RIGHT_AXIS_Y_NEG), "Right Stick Up"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(RIGHT_AXIS_Y_POS), "Right Stick Down"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(RIGHT_AXIS_X_POS), "Right Stick Right"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(RIGHT_AXIS_X_NEG), "Right Stick Left"));

        availableInputs.append(UserInputMapper::InputPair(makeInput(LEFT_HAND), "Left Hand"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(RIGHT_HAND), "Right Hand"));

        return availableInputs;
    };
    proxy->resetDeviceBindings = [this, &mapper] () -> bool {
        mapper.removeAllInputChannelsForDevice(_deviceID);
        this->assignDefaultInputMapping(mapper);
        return true;
    };
    mapper.registerStandardDevice(proxy);
}

void StandardController::assignDefaultInputMapping(UserInputMapper& mapper) {
    const float JOYSTICK_MOVE_SPEED = 1.0f;
    const float DPAD_MOVE_SPEED = 0.5f;
    const float JOYSTICK_YAW_SPEED = 0.5f;
    const float JOYSTICK_PITCH_SPEED = 0.25f;
    const float BOOM_SPEED = 0.1f;
    
    // Y axes are flipped (up is negative)
    // Left StandardController: Movement, strafing
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_FORWARD, makeInput(LEFT_AXIS_Y_NEG), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_BACKWARD, makeInput(LEFT_AXIS_Y_POS), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LATERAL_RIGHT, makeInput(LEFT_AXIS_X_POS), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LATERAL_LEFT, makeInput(LEFT_AXIS_X_NEG), JOYSTICK_MOVE_SPEED);
    
    // Right StandardController: Camera orientation
    mapper.addInputChannel(UserInputMapper::YAW_RIGHT, makeInput(RIGHT_AXIS_X_POS), JOYSTICK_YAW_SPEED);
    mapper.addInputChannel(UserInputMapper::YAW_LEFT, makeInput(RIGHT_AXIS_X_NEG), JOYSTICK_YAW_SPEED);
    mapper.addInputChannel(UserInputMapper::PITCH_UP, makeInput(RIGHT_AXIS_Y_NEG), JOYSTICK_PITCH_SPEED);
    mapper.addInputChannel(UserInputMapper::PITCH_DOWN, makeInput(RIGHT_AXIS_Y_POS), JOYSTICK_PITCH_SPEED);
    
    // Dpad movement
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_FORWARD, makeInput(STANDARD_CONTROLLER_BUTTON_DPAD_UP), DPAD_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_BACKWARD, makeInput(STANDARD_CONTROLLER_BUTTON_DPAD_DOWN), DPAD_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LATERAL_RIGHT, makeInput(STANDARD_CONTROLLER_BUTTON_DPAD_RIGHT), DPAD_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LATERAL_LEFT, makeInput(STANDARD_CONTROLLER_BUTTON_DPAD_LEFT), DPAD_MOVE_SPEED);
    
    // Button controls
    mapper.addInputChannel(UserInputMapper::VERTICAL_UP, makeInput(STANDARD_CONTROLLER_BUTTON_Y), DPAD_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::VERTICAL_DOWN, makeInput(STANDARD_CONTROLLER_BUTTON_X), DPAD_MOVE_SPEED);
    
    // Zoom
    mapper.addInputChannel(UserInputMapper::BOOM_IN, makeInput(RIGHT_SHOULDER), BOOM_SPEED);
    mapper.addInputChannel(UserInputMapper::BOOM_OUT, makeInput(LEFT_SHOULDER), BOOM_SPEED);
    
    
    // Hold front right shoulder button for precision controls
    // Left StandardController: Movement, strafing
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_FORWARD, makeInput(LEFT_AXIS_Y_NEG), makeInput(STANDARD_CONTROLLER_BUTTON_RIGHTSHOULDER), JOYSTICK_MOVE_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_BACKWARD, makeInput(LEFT_AXIS_Y_POS), makeInput(STANDARD_CONTROLLER_BUTTON_RIGHTSHOULDER), JOYSTICK_MOVE_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::LATERAL_RIGHT, makeInput(LEFT_AXIS_X_POS), makeInput(STANDARD_CONTROLLER_BUTTON_RIGHTSHOULDER), JOYSTICK_MOVE_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::LATERAL_LEFT, makeInput(LEFT_AXIS_X_NEG), makeInput(STANDARD_CONTROLLER_BUTTON_RIGHTSHOULDER), JOYSTICK_MOVE_SPEED/2.0f);
    
    // Right StandardController: Camera orientation
    mapper.addInputChannel(UserInputMapper::YAW_RIGHT, makeInput(RIGHT_AXIS_X_POS), makeInput(STANDARD_CONTROLLER_BUTTON_RIGHTSHOULDER), JOYSTICK_YAW_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::YAW_LEFT, makeInput(RIGHT_AXIS_X_NEG), makeInput(STANDARD_CONTROLLER_BUTTON_RIGHTSHOULDER), JOYSTICK_YAW_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::PITCH_UP, makeInput(RIGHT_AXIS_Y_NEG), makeInput(STANDARD_CONTROLLER_BUTTON_RIGHTSHOULDER), JOYSTICK_PITCH_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::PITCH_DOWN, makeInput(RIGHT_AXIS_Y_POS), makeInput(STANDARD_CONTROLLER_BUTTON_RIGHTSHOULDER), JOYSTICK_PITCH_SPEED/2.0f);
    
    // Dpad movement
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_FORWARD, makeInput(STANDARD_CONTROLLER_BUTTON_DPAD_UP), makeInput(STANDARD_CONTROLLER_BUTTON_RIGHTSHOULDER), DPAD_MOVE_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_BACKWARD, makeInput(STANDARD_CONTROLLER_BUTTON_DPAD_DOWN), makeInput(STANDARD_CONTROLLER_BUTTON_RIGHTSHOULDER), DPAD_MOVE_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::LATERAL_RIGHT, makeInput(STANDARD_CONTROLLER_BUTTON_DPAD_RIGHT), makeInput(STANDARD_CONTROLLER_BUTTON_RIGHTSHOULDER), DPAD_MOVE_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::LATERAL_LEFT, makeInput(STANDARD_CONTROLLER_BUTTON_DPAD_LEFT), makeInput(STANDARD_CONTROLLER_BUTTON_RIGHTSHOULDER), DPAD_MOVE_SPEED/2.0f);
    
    // Button controls
    mapper.addInputChannel(UserInputMapper::VERTICAL_UP, makeInput(STANDARD_CONTROLLER_BUTTON_Y), makeInput(STANDARD_CONTROLLER_BUTTON_RIGHTSHOULDER), DPAD_MOVE_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::VERTICAL_DOWN, makeInput(STANDARD_CONTROLLER_BUTTON_X), makeInput(STANDARD_CONTROLLER_BUTTON_RIGHTSHOULDER), DPAD_MOVE_SPEED/2.0f);
    
    // Zoom
    mapper.addInputChannel(UserInputMapper::BOOM_IN, makeInput(RIGHT_SHOULDER), makeInput(STANDARD_CONTROLLER_BUTTON_RIGHTSHOULDER), BOOM_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::BOOM_OUT, makeInput(LEFT_SHOULDER), makeInput(STANDARD_CONTROLLER_BUTTON_RIGHTSHOULDER), BOOM_SPEED/2.0f);
    
    mapper.addInputChannel(UserInputMapper::SHIFT, makeInput(STANDARD_CONTROLLER_BUTTON_LEFTSHOULDER));
    
    mapper.addInputChannel(UserInputMapper::ACTION1, makeInput(STANDARD_CONTROLLER_BUTTON_B));
    mapper.addInputChannel(UserInputMapper::ACTION2, makeInput(STANDARD_CONTROLLER_BUTTON_A));
}

UserInputMapper::Input StandardController::makeInput(StandardController::StandardControllerButtonChannel button) {
    return UserInputMapper::Input(_deviceID, button, UserInputMapper::ChannelType::BUTTON);
}

UserInputMapper::Input StandardController::makeInput(StandardController::StandardControllerAxisChannel axis) {
    return UserInputMapper::Input(_deviceID, axis, UserInputMapper::ChannelType::AXIS);
}

UserInputMapper::Input StandardController::makeInput(StandardController::StandardControllerPoseChannel pose) {
    return UserInputMapper::Input(_deviceID, pose, UserInputMapper::ChannelType::POSE);
}
