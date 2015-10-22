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
        availableInputs.append(makePair(A, "A"));
        availableInputs.append(makePair(B, "B"));
        availableInputs.append(makePair(X, "X"));
        availableInputs.append(makePair(Y, "Y"));

        // DPad
        availableInputs.append(makePair(DU, "DU"));
        availableInputs.append(makePair(DD, "DD"));
        availableInputs.append(makePair(DL, "DL"));
        availableInputs.append(makePair(DR, "DR"));

        // Bumpers
        availableInputs.append(makePair(LB, "LB"));
        availableInputs.append(makePair(RB, "RB"));

        // Stick press
        availableInputs.append(makePair(LS, "LS"));
        availableInputs.append(makePair(RS, "RS"));

        // Center buttons
        availableInputs.append(makePair(START, "Start"));
        availableInputs.append(makePair(BACK, "Back"));

        // Analog sticks
        availableInputs.append(makePair(LY, "LY"));
        availableInputs.append(makePair(LX, "LX"));
        availableInputs.append(makePair(RY, "RY"));
        availableInputs.append(makePair(RX, "RX"));

        // Triggers
        availableInputs.append(makePair(LT, "LT"));
        availableInputs.append(makePair(RT, "RT"));


        // Finger abstractions
        availableInputs.append(makePair(LEFT_PRIMARY_THUMB, "LeftPrimaryThumb"));
        availableInputs.append(makePair(LEFT_SECONDARY_THUMB, "LeftSecondaryThumb"));
        availableInputs.append(makePair(RIGHT_PRIMARY_THUMB, "RightPrimaryThumb"));
        availableInputs.append(makePair(RIGHT_SECONDARY_THUMB, "RightSecondaryThumb"));

        availableInputs.append(makePair(LEFT_PRIMARY_INDEX, "LeftPrimaryIndex"));
        availableInputs.append(makePair(LEFT_SECONDARY_INDEX, "LeftSecondaryIndex"));
        availableInputs.append(makePair(RIGHT_PRIMARY_INDEX, "RightPrimaryIndex"));
        availableInputs.append(makePair(RIGHT_SECONDARY_INDEX, "RightSecondaryIndex"));

        availableInputs.append(makePair(LEFT_GRIP, "LeftGrip"));
        availableInputs.append(makePair(RIGHT_GRIP, "RightGrip"));

        // Poses
        availableInputs.append(makePair(LEFT_HAND, "LeftHand"));
        availableInputs.append(makePair(RIGHT_HAND, "RightHand"));


        // Aliases, PlayStation style names
        availableInputs.append(makePair(LB, "L1"));
        availableInputs.append(makePair(RB, "R1"));
        availableInputs.append(makePair(LT, "L2"));
        availableInputs.append(makePair(RT, "R2"));
        availableInputs.append(makePair(LS, "L3"));
        availableInputs.append(makePair(RS, "R3"));
        availableInputs.append(makePair(BACK, "Select"));
        availableInputs.append(makePair(A, "Cross"));
        availableInputs.append(makePair(B, "Circle"));
        availableInputs.append(makePair(X, "Square"));
        availableInputs.append(makePair(Y, "Triangle"));
        availableInputs.append(makePair(DU, "Up"));
        availableInputs.append(makePair(DD, "Down"));
        availableInputs.append(makePair(DL, "Left"));
        availableInputs.append(makePair(DR, "Right"));



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
