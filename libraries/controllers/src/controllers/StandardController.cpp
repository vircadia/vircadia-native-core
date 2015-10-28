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

#include "UserInputMapper.h"
#include "impl/endpoints/StandardEndpoint.h"

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

Input::NamedVector StandardController::getAvailableInputs() const {
    static Input::NamedVector availableInputs {
        // Buttons
        makePair(A, "A"),
        makePair(B, "B"),
        makePair(X, "X"),
        makePair(Y, "Y"),

        // DPad
        makePair(DU, "DU"),
        makePair(DD, "DD"),
        makePair(DL, "DL"),
        makePair(DR, "DR"),

        // Bumpers
        makePair(LB, "LB"),
        makePair(RB, "RB"),

        // Stick press
        makePair(LS, "LS"),
        makePair(RS, "RS"),

        // Center buttons
        makePair(START, "Start"),
        makePair(BACK, "Back"),

        // Analog sticks
        makePair(LY, "LY"),
        makePair(LX, "LX"),
        makePair(RY, "RY"),
        makePair(RX, "RX"),

        // Triggers
        makePair(LT, "LT"),
        makePair(RT, "RT"),


        // Finger abstractions
        makePair(LEFT_PRIMARY_THUMB, "LeftPrimaryThumb"),
        makePair(LEFT_SECONDARY_THUMB, "LeftSecondaryThumb"),
        makePair(RIGHT_PRIMARY_THUMB, "RightPrimaryThumb"),
        makePair(RIGHT_SECONDARY_THUMB, "RightSecondaryThumb"),

        makePair(LEFT_PRIMARY_INDEX, "LeftPrimaryIndex"),
        makePair(LEFT_SECONDARY_INDEX, "LeftSecondaryIndex"),
        makePair(RIGHT_PRIMARY_INDEX, "RightPrimaryIndex"),
        makePair(RIGHT_SECONDARY_INDEX, "RightSecondaryIndex"),

        makePair(LEFT_GRIP, "LeftGrip"),
        makePair(RIGHT_GRIP, "RightGrip"),

        // Poses
        makePair(LEFT_HAND, "LeftHand"),
        makePair(RIGHT_HAND, "RightHand"),


        // Aliases, PlayStation style names
        makePair(LB, "L1"),
        makePair(RB, "R1"),
        makePair(LT, "L2"),
        makePair(RT, "R2"),
        makePair(LS, "L3"),
        makePair(RS, "R3"),
        makePair(BACK, "Select"),
        makePair(A, "Cross"),
        makePair(B, "Circle"),
        makePair(X, "Square"),
        makePair(Y, "Triangle"),
        makePair(DU, "Up"),
        makePair(DD, "Down"),
        makePair(DL, "Left"),
        makePair(DR, "Right"),
    };
    return availableInputs;
}

EndpointPointer StandardController::createEndpoint(const Input& input) const {
    return std::make_shared<StandardEndpoint>(input);
}

QString StandardController::getDefaultMappingConfig() const {
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
