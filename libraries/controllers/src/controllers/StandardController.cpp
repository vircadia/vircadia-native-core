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

StandardController::~StandardController() {
}

void StandardController::update(float deltaTime, bool jointsCaptured) {
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
        // Buttons
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::A), "A"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::B), "B"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::X), "X"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::Y), "Y"));

        // DPad
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::DU), "DU"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::DD), "DD"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::DL), "DL"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::DR), "DR"));

        // Bumpers
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::LB), "LB"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::RB), "RB"));

        // Stick press
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::LS), "LS"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::RS), "RS"));

        // Center buttons
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::START), "Start"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::BACK), "Back"));

        // Analog sticks
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::LY), "LY"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::LX), "LX"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::RY), "RY"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::RX), "RX"));

        // Triggers
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::LT), "LT"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::RT), "RT"));

        // Poses
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::LEFT_HAND), "LeftHand"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::RIGHT_HAND), "RightHand"));

        // Aliases, PlayStation style names
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::LB), "L1"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::RB), "R1"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::LT), "L2"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::RT), "R2"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::LS), "L3"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::RS), "R3"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::BACK), "Select"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::A), "Cross"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::B), "Circle"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::X), "Square"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::Y), "Triangle"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::DU), "Up"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::DD), "Down"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::DL), "Left"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::DR), "Right"));


        return availableInputs;
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
    // Left Joystick: Movement, strafing
    mapper.addInputChannel(UserInputMapper::TRANSLATE_Z, makeInput(controller::LY), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::TRANSLATE_X, makeInput(controller::LX), JOYSTICK_MOVE_SPEED);
    // Right Joystick: Camera orientation
    mapper.addInputChannel(UserInputMapper::YAW, makeInput(controller::RX), JOYSTICK_YAW_SPEED);
    mapper.addInputChannel(UserInputMapper::PITCH, makeInput(controller::RY), JOYSTICK_PITCH_SPEED);

    // Dpad movement
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_FORWARD, makeInput(controller::DU), DPAD_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_BACKWARD, makeInput(controller::DD), DPAD_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LATERAL_RIGHT, makeInput(controller::DR), DPAD_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LATERAL_LEFT, makeInput(controller::DL), DPAD_MOVE_SPEED);

    // Button controls
    mapper.addInputChannel(UserInputMapper::VERTICAL_UP, makeInput(controller::Y), DPAD_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::VERTICAL_DOWN, makeInput(controller::X), DPAD_MOVE_SPEED);

    // Zoom
    mapper.addInputChannel(UserInputMapper::BOOM_IN, makeInput(controller::RT), BOOM_SPEED);
    mapper.addInputChannel(UserInputMapper::BOOM_OUT, makeInput(controller::LB), BOOM_SPEED);

    // Hold front right shoulder button for precision controls
    // Left Joystick: Movement, strafing
    mapper.addInputChannel(UserInputMapper::TRANSLATE_Z, makeInput(controller::LY), makeInput(controller::RB), JOYSTICK_MOVE_SPEED / 2.0f);
    mapper.addInputChannel(UserInputMapper::TRANSLATE_X, makeInput(controller::LY), makeInput(controller::RB), JOYSTICK_MOVE_SPEED / 2.0f);

    // Right Joystick: Camera orientation
    mapper.addInputChannel(UserInputMapper::YAW, makeInput(controller::RX), makeInput(controller::RB), JOYSTICK_YAW_SPEED / 2.0f);
    mapper.addInputChannel(UserInputMapper::PITCH, makeInput(controller::RY), makeInput(controller::RB), JOYSTICK_PITCH_SPEED / 2.0f);

    // Dpad movement
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_FORWARD, makeInput(controller::DU), makeInput(controller::RB), DPAD_MOVE_SPEED / 2.0f);
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_BACKWARD, makeInput(controller::DD), makeInput(controller::RB), DPAD_MOVE_SPEED / 2.0f);
    mapper.addInputChannel(UserInputMapper::LATERAL_RIGHT, makeInput(controller::DR), makeInput(controller::RB), DPAD_MOVE_SPEED / 2.0f);
    mapper.addInputChannel(UserInputMapper::LATERAL_LEFT, makeInput(controller::DL), makeInput(controller::RB), DPAD_MOVE_SPEED / 2.0f);

    // Button controls
    mapper.addInputChannel(UserInputMapper::VERTICAL_UP, makeInput(controller::Y), makeInput(controller::RB), DPAD_MOVE_SPEED / 2.0f);
    mapper.addInputChannel(UserInputMapper::VERTICAL_DOWN, makeInput(controller::X), makeInput(controller::RB), DPAD_MOVE_SPEED / 2.0f);

    // Zoom
    mapper.addInputChannel(UserInputMapper::BOOM_IN, makeInput(controller::RT), makeInput(controller::RB), BOOM_SPEED / 2.0f);
    mapper.addInputChannel(UserInputMapper::BOOM_OUT, makeInput(controller::LT), makeInput(controller::RB), BOOM_SPEED / 2.0f);

    mapper.addInputChannel(UserInputMapper::SHIFT, makeInput(controller::RB));

    mapper.addInputChannel(UserInputMapper::ACTION1, makeInput(controller::B));
    mapper.addInputChannel(UserInputMapper::ACTION2, makeInput(controller::A));
}

UserInputMapper::Input StandardController::makeInput(controller::StandardButtonChannel button) {
    return UserInputMapper::Input(_deviceID, button, UserInputMapper::ChannelType::BUTTON);
}

UserInputMapper::Input StandardController::makeInput(controller::StandardAxisChannel axis) {
    return UserInputMapper::Input(_deviceID, axis, UserInputMapper::ChannelType::AXIS);
}

UserInputMapper::Input StandardController::makeInput(controller::StandardPoseChannel pose) {
    return UserInputMapper::Input(_deviceID, pose, UserInputMapper::ChannelType::POSE);
}
