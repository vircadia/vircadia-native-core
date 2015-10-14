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
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::LEFT), "LeftPose"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::RIGHT), "RightPose"));

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

    proxy->resetDeviceBindings = [this, &mapper] () -> bool {
        mapper.removeAllInputChannelsForDevice(_deviceID);
        this->assignDefaultInputMapping(mapper);
        return true;
    };

    mapper.registerStandardDevice(proxy);
}

void StandardController::assignDefaultInputMapping(UserInputMapper& mapper) {
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
