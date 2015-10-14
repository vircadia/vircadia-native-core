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

#include <limits>
#include <glm/glm.hpp>

const float CONTROLLER_THRESHOLD = 0.3f;

#ifdef HAVE_SDL2
const float MAX_AXIS = 32768.0f;

Joystick::Joystick(SDL_JoystickID instanceId, const QString& name, SDL_GameController* sdlGameController) :
        InputDevice(name),
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

void Joystick::update(float deltaTime, bool jointsCaptured) {
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


void Joystick::registerToUserInputMapper(UserInputMapper& mapper) {
    // Grab the current free device ID
    _deviceID = mapper.getFreeDeviceID();
    
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
    mapper.registerDevice(_deviceID, proxy);
}

void Joystick::assignDefaultInputMapping(UserInputMapper& mapper) {
#ifdef HAVE_SDL2
    const float JOYSTICK_MOVE_SPEED = 1.0f;
    const float DPAD_MOVE_SPEED = 0.5f;
    const float JOYSTICK_YAW_SPEED = 0.5f;
    const float JOYSTICK_PITCH_SPEED = 0.25f;
    const float BOOM_SPEED = 0.1f;

#endif
}

UserInputMapper::Input Joystick::makeInput(controller::StandardButtonChannel button) {
    return UserInputMapper::Input(_deviceID, button, UserInputMapper::ChannelType::BUTTON);
}

UserInputMapper::Input Joystick::makeInput(controller::StandardAxisChannel axis) {
    return UserInputMapper::Input(_deviceID, axis, UserInputMapper::ChannelType::AXIS);
}

