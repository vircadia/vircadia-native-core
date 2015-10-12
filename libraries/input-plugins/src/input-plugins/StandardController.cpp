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
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::A), "A"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::B), "B"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::X), "X"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::Y), "Y"));

        // DPad
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::DU), "DU"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::DD), "DD"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::DL), "DL"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::DR), "DR"));

        // Bumpers
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::LB), "LB"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::RB), "RB"));

        // Stick press
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::LS), "LS"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::RS), "RS"));

        // Center buttons
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::START), "Start"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::BACK), "Back"));

        // Analog sticks
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::LY), "LY"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::LX), "LX"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::RY), "RY"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::RX), "RX"));

        // Triggers
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::LT), "LT"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::RT), "RT"));

        // Poses
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::LeftPose), "LeftPose"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::RightPose), "RightPose"));

        // Aliases, PlayStation style names
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::LB), "L1"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::RB), "R1"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::LT), "L2"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::RT), "R2"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::LS), "L3"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::RS), "R3"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::BACK), "Select"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::A), "Cross"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::B), "Circle"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::X), "Square"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::Y), "Triangle"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::DU), "Up"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::DD), "Down"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::DL), "Left"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(Controllers::DR), "Right"));


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

UserInputMapper::Input StandardController::makeInput(Controllers::StandardButtonChannel button) {
    return UserInputMapper::Input(_deviceID, button, UserInputMapper::ChannelType::BUTTON);
}

UserInputMapper::Input StandardController::makeInput(Controllers::StandardAxisChannel axis) {
    return UserInputMapper::Input(_deviceID, axis, UserInputMapper::ChannelType::AXIS);
}

UserInputMapper::Input StandardController::makeInput(Controllers::StandardPoseChannel pose) {
    return UserInputMapper::Input(_deviceID, pose, UserInputMapper::ChannelType::POSE);
}
