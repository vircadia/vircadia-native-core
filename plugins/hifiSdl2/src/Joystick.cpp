//
//  Joystick.cpp
//  input-plugins/src/input-plugins
//
//  Created by Stephen Birarda on 2014-09-23.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Joystick.h"

#include <PathUtils.h>

const float CONTROLLER_THRESHOLD = 0.3f;

#ifdef HAVE_SDL2
const float MAX_AXIS = 32768.0f;

Joystick::Joystick(SDL_JoystickID instanceId, SDL_GameController* sdlGameController) :
        InputDevice("GamePad"),
    _sdlGameController(sdlGameController),
    _sdlJoystick(SDL_GameControllerGetJoystick(_sdlGameController)),
    _instanceId(instanceId)
{
    
}

#endif

Joystick::~Joystick() {
    closeJoystick();
}

void Joystick::closeJoystick() {
#ifdef HAVE_SDL2
    SDL_GameControllerClose(_sdlGameController);
#endif
}

void Joystick::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, bool jointsCaptured) {
    for (auto axisState : _axisStateMap) {
        if (fabsf(axisState.second) < CONTROLLER_THRESHOLD) {
            _axisStateMap[axisState.first] = 0.0f;
        }
    }
}

void Joystick::focusOutEvent() {
    _axisStateMap.clear();
    _buttonPressedMap.clear();
};

#ifdef HAVE_SDL2

void Joystick::handleAxisEvent(const SDL_ControllerAxisEvent& event) {
    SDL_GameControllerAxis axis = (SDL_GameControllerAxis) event.axis;
    _axisStateMap[makeInput((controller::StandardAxisChannel)axis).getChannel()] = (float)event.value / MAX_AXIS;
}

void Joystick::handleButtonEvent(const SDL_ControllerButtonEvent& event) {
    auto input = makeInput((controller::StandardButtonChannel)event.button);
    bool newValue = event.state == SDL_PRESSED;
    if (newValue) {
        _buttonPressedMap.insert(input.getChannel());
    } else {
        _buttonPressedMap.erase(input.getChannel());
    }
}

#endif

controller::Input::NamedVector Joystick::getAvailableInputs() const {
    using namespace controller;
    static const Input::NamedVector availableInputs{
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
        makePair(LX, "LX"),
        makePair(LY, "LY"),
        makePair(RX, "RX"),
        makePair(RY, "RY"),
 
        // Triggers
        makePair(LT, "LT"),
        makePair(RT, "RT"),

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

QString Joystick::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/xbox.json";
    return MAPPING_JSON;
}
