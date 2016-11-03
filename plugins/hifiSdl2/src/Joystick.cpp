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

const float MAX_AXIS = 32768.0f;

Joystick::Joystick(SDL_JoystickID instanceId, SDL_GameController* sdlGameController) :
        InputDevice("GamePad"),
    _sdlGameController(sdlGameController),
    _sdlJoystick(SDL_GameControllerGetJoystick(_sdlGameController)),
    _sdlHaptic(SDL_HapticOpenFromJoystick(_sdlJoystick)),
    _instanceId(instanceId)
{
    if (!_sdlHaptic) {
        qDebug() << "SDL Haptic Open Failure: " << QString(SDL_GetError());
    } else {
        if (SDL_HapticRumbleInit(_sdlHaptic) != 0) {
            qDebug() << "SDL Haptic Rumble Init Failure: " << QString(SDL_GetError());
        }
    }
}

Joystick::~Joystick() {
    closeJoystick();
}

void Joystick::closeJoystick() {
    if (_sdlHaptic) {
        SDL_HapticClose(_sdlHaptic);
    }
    SDL_GameControllerClose(_sdlGameController);
}

void Joystick::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
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

bool Joystick::triggerHapticPulse(float strength, float duration, controller::Hand hand) {
    if (SDL_HapticRumblePlay(_sdlHaptic, strength, duration) != 0) {
        return false;
    }
    return true;
}

controller::Input::NamedVector Joystick::getAvailableInputs() const {
    using namespace controller;
    if (_availableInputs.length() == 0) {
        _availableInputs = {
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
    }
    return _availableInputs;
}

QString Joystick::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/xbox.json";
    return MAPPING_JSON;
}
