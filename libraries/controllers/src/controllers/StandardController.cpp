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

#include "StandardController.h"

#include <PathUtils.h>

#include "DeviceProxy.h"
#include "UserInputMapper.h"

namespace controller {

StandardController::StandardController() : InputDevice("Standard") { 
    _deviceID = UserInputMapper::STANDARD_DEVICE; 
}

StandardController::~StandardController() {
}

void StandardController::update(float deltaTime, bool jointsCaptured) {
}

void StandardController::focusOutEvent() {
    _axisStateMap.clear();
    _buttonPressedMap.clear();
};

void StandardController::buildDeviceProxy(DeviceProxy::Pointer proxy) {
    proxy->_name = _name;
    proxy->getButton = [this] (const Input& input, int timestamp) -> bool { return getButton(input.getChannel()); };
    proxy->getAxis = [this] (const Input& input, int timestamp) -> float { return getAxis(input.getChannel()); };
    proxy->getAvailabeInputs = [this] () -> QVector<Input::NamedPair> {
        QVector<Input::NamedPair> availableInputs;
        // Buttons
        availableInputs.append(Input::NamedPair(makeInput(controller::A), "A"));
        availableInputs.append(Input::NamedPair(makeInput(controller::B), "B"));
        availableInputs.append(Input::NamedPair(makeInput(controller::X), "X"));
        availableInputs.append(Input::NamedPair(makeInput(controller::Y), "Y"));

        // DPad
        availableInputs.append(Input::NamedPair(makeInput(controller::DU), "DU"));
        availableInputs.append(Input::NamedPair(makeInput(controller::DD), "DD"));
        availableInputs.append(Input::NamedPair(makeInput(controller::DL), "DL"));
        availableInputs.append(Input::NamedPair(makeInput(controller::DR), "DR"));

        // Bumpers
        availableInputs.append(Input::NamedPair(makeInput(controller::LB), "LB"));
        availableInputs.append(Input::NamedPair(makeInput(controller::RB), "RB"));

        // Stick press
        availableInputs.append(Input::NamedPair(makeInput(controller::LS), "LS"));
        availableInputs.append(Input::NamedPair(makeInput(controller::RS), "RS"));

        // Center buttons
        availableInputs.append(Input::NamedPair(makeInput(controller::START), "Start"));
        availableInputs.append(Input::NamedPair(makeInput(controller::BACK), "Back"));

        // Analog sticks
        availableInputs.append(Input::NamedPair(makeInput(controller::LY), "LY"));
        availableInputs.append(Input::NamedPair(makeInput(controller::LX), "LX"));
        availableInputs.append(Input::NamedPair(makeInput(controller::RY), "RY"));
        availableInputs.append(Input::NamedPair(makeInput(controller::RX), "RX"));

        // Triggers
        availableInputs.append(Input::NamedPair(makeInput(controller::LT), "LT"));
        availableInputs.append(Input::NamedPair(makeInput(controller::RT), "RT"));

        // Poses
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::LEFT_HAND), "LeftHand"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(controller::RIGHT_HAND), "RightHand"));

        // Aliases, PlayStation style names
        availableInputs.append(Input::NamedPair(makeInput(controller::LB), "L1"));
        availableInputs.append(Input::NamedPair(makeInput(controller::RB), "R1"));
        availableInputs.append(Input::NamedPair(makeInput(controller::LT), "L2"));
        availableInputs.append(Input::NamedPair(makeInput(controller::RT), "R2"));
        availableInputs.append(Input::NamedPair(makeInput(controller::LS), "L3"));
        availableInputs.append(Input::NamedPair(makeInput(controller::RS), "R3"));
        availableInputs.append(Input::NamedPair(makeInput(controller::BACK), "Select"));
        availableInputs.append(Input::NamedPair(makeInput(controller::A), "Cross"));
        availableInputs.append(Input::NamedPair(makeInput(controller::B), "Circle"));
        availableInputs.append(Input::NamedPair(makeInput(controller::X), "Square"));
        availableInputs.append(Input::NamedPair(makeInput(controller::Y), "Triangle"));
        availableInputs.append(Input::NamedPair(makeInput(controller::DU), "Up"));
        availableInputs.append(Input::NamedPair(makeInput(controller::DD), "Down"));
        availableInputs.append(Input::NamedPair(makeInput(controller::DL), "Left"));
        availableInputs.append(Input::NamedPair(makeInput(controller::DR), "Right"));
        return availableInputs;
    };
}


QString StandardController::getDefaultMappingConfig() {
    static const QString DEFAULT_MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/standard.json";
    return DEFAULT_MAPPING_JSON;
}

// FIXME figure out how to move the shifted version to JSON
//void StandardController::assignDefaultInputMapping(UserInputMapper& mapper) {
//    const float JOYSTICK_MOVE_SPEED = 1.0f;
//    const float DPAD_MOVE_SPEED = 0.5f;
//    const float JOYSTICK_YAW_SPEED = 0.5f;
//    const float JOYSTICK_PITCH_SPEED = 0.25f;
//    const float BOOM_SPEED = 0.1f;
//
//    // Hold front right shoulder button for precision controls
//    // Left Joystick: Movement, strafing
//    mapper.addInputChannel(UserInputMapper::TRANSLATE_Z, makeInput(controller::LY), makeInput(controller::RB), JOYSTICK_MOVE_SPEED / 2.0f);
//    mapper.addInputChannel(UserInputMapper::TRANSLATE_X, makeInput(controller::LY), makeInput(controller::RB), JOYSTICK_MOVE_SPEED / 2.0f);
//
//    // Right Joystick: Camera orientation
//    mapper.addInputChannel(UserInputMapper::YAW, makeInput(controller::RX), makeInput(controller::RB), JOYSTICK_YAW_SPEED / 2.0f);
//    mapper.addInputChannel(UserInputMapper::PITCH, makeInput(controller::RY), makeInput(controller::RB), JOYSTICK_PITCH_SPEED / 2.0f);
//
//    // Dpad movement
//    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_FORWARD, makeInput(controller::DU), makeInput(controller::RB), DPAD_MOVE_SPEED / 2.0f);
//    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_BACKWARD, makeInput(controller::DD), makeInput(controller::RB), DPAD_MOVE_SPEED / 2.0f);
//    mapper.addInputChannel(UserInputMapper::LATERAL_RIGHT, makeInput(controller::DR), makeInput(controller::RB), DPAD_MOVE_SPEED / 2.0f);
//    mapper.addInputChannel(UserInputMapper::LATERAL_LEFT, makeInput(controller::DL), makeInput(controller::RB), DPAD_MOVE_SPEED / 2.0f);
//
//    // Button controls
//    mapper.addInputChannel(UserInputMapper::VERTICAL_UP, makeInput(controller::Y), makeInput(controller::RB), DPAD_MOVE_SPEED / 2.0f);
//    mapper.addInputChannel(UserInputMapper::VERTICAL_DOWN, makeInput(controller::X), makeInput(controller::RB), DPAD_MOVE_SPEED / 2.0f);
//
//    // Zoom
//    mapper.addInputChannel(UserInputMapper::BOOM_IN, makeInput(controller::RT), makeInput(controller::RB), BOOM_SPEED / 2.0f);
//    mapper.addInputChannel(UserInputMapper::BOOM_OUT, makeInput(controller::LT), makeInput(controller::RB), BOOM_SPEED / 2.0f);
//
//    mapper.addInputChannel(UserInputMapper::SHIFT, makeInput(controller::RB));
//
//    mapper.addInputChannel(UserInputMapper::ACTION1, makeInput(controller::B));
//    mapper.addInputChannel(UserInputMapper::ACTION2, makeInput(controller::A));
//}

}
