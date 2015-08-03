//
//  Joystick.cpp
//  interface/src/devices
//
//  Created by Stephen Birarda on 2014-09-23.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <limits>

#include <glm/glm.hpp>

#include "Application.h"

#include "Joystick.h"

const float CONTROLLER_THRESHOLD = 0.3f;

#ifdef HAVE_SDL2
const float MAX_AXIS = 32768.0f;

Joystick::Joystick(SDL_JoystickID instanceId, const QString& name, SDL_GameController* sdlGameController) :
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

void Joystick::update() {
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
    
    switch (axis) {
        case SDL_CONTROLLER_AXIS_LEFTX:
            _axisStateMap[makeInput(LEFT_AXIS_X_POS).getChannel()] = (event.value > 0.0f) ? event.value / MAX_AXIS : 0.0f;
            _axisStateMap[makeInput(LEFT_AXIS_X_NEG).getChannel()] = (event.value < 0.0f) ? -event.value / MAX_AXIS : 0.0f;
            break;
        case SDL_CONTROLLER_AXIS_LEFTY:
            _axisStateMap[makeInput(LEFT_AXIS_Y_POS).getChannel()] = (event.value > 0.0f) ? event.value / MAX_AXIS : 0.0f;
            _axisStateMap[makeInput(LEFT_AXIS_Y_NEG).getChannel()] = (event.value < 0.0f) ? -event.value / MAX_AXIS : 0.0f;
            break;
        case SDL_CONTROLLER_AXIS_RIGHTX:
            _axisStateMap[makeInput(RIGHT_AXIS_X_POS).getChannel()] = (event.value > 0.0f) ? event.value / MAX_AXIS : 0.0f;
            _axisStateMap[makeInput(RIGHT_AXIS_X_NEG).getChannel()] = (event.value < 0.0f) ? -event.value / MAX_AXIS : 0.0f;
            break;
        case SDL_CONTROLLER_AXIS_RIGHTY:
            _axisStateMap[makeInput(RIGHT_AXIS_Y_POS).getChannel()] = (event.value > 0.0f) ? event.value / MAX_AXIS : 0.0f;
            _axisStateMap[makeInput(RIGHT_AXIS_Y_NEG).getChannel()] = (event.value < 0.0f) ? -event.value / MAX_AXIS : 0.0f;
            break;
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
            _axisStateMap[makeInput(RIGHT_SHOULDER).getChannel()] = event.value / MAX_AXIS;
            break;
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
            _axisStateMap[makeInput(LEFT_SHOULDER).getChannel()] = event.value / MAX_AXIS;
            break;
        default:
            break;
    }
}

void Joystick::handleButtonEvent(const SDL_ControllerButtonEvent& event) {
    auto input = makeInput((SDL_GameControllerButton) event.button);
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
#ifdef HAVE_SDL2
        availableInputs.append(UserInputMapper::InputPair(makeInput(SDL_CONTROLLER_BUTTON_A), "Bottom Button"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(SDL_CONTROLLER_BUTTON_B), "Right Button"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(SDL_CONTROLLER_BUTTON_X), "Left Button"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(SDL_CONTROLLER_BUTTON_Y), "Top Button"));
        
        availableInputs.append(UserInputMapper::InputPair(makeInput(SDL_CONTROLLER_BUTTON_DPAD_UP), "DPad Up"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(SDL_CONTROLLER_BUTTON_DPAD_DOWN), "DPad Down"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(SDL_CONTROLLER_BUTTON_DPAD_LEFT), "DPad Left"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(SDL_CONTROLLER_BUTTON_DPAD_RIGHT), "DPad Right"));
        
        availableInputs.append(UserInputMapper::InputPair(makeInput(SDL_CONTROLLER_BUTTON_LEFTSHOULDER), "L1"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER), "R1"));
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

#endif
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
    
    // Y axes are flipped (up is negative)
    // Left Joystick: Movement, strafing
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_FORWARD, makeInput(LEFT_AXIS_Y_NEG), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_BACKWARD, makeInput(LEFT_AXIS_Y_POS), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LATERAL_RIGHT, makeInput(LEFT_AXIS_X_POS), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LATERAL_LEFT, makeInput(LEFT_AXIS_X_NEG), JOYSTICK_MOVE_SPEED);
    
    // Right Joystick: Camera orientation
    mapper.addInputChannel(UserInputMapper::YAW_RIGHT, makeInput(RIGHT_AXIS_X_POS), JOYSTICK_YAW_SPEED);
    mapper.addInputChannel(UserInputMapper::YAW_LEFT, makeInput(RIGHT_AXIS_X_NEG), JOYSTICK_YAW_SPEED);
    mapper.addInputChannel(UserInputMapper::PITCH_UP, makeInput(RIGHT_AXIS_Y_NEG), JOYSTICK_PITCH_SPEED);
    mapper.addInputChannel(UserInputMapper::PITCH_DOWN, makeInput(RIGHT_AXIS_Y_POS), JOYSTICK_PITCH_SPEED);
    
    // Dpad movement
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_FORWARD, makeInput(SDL_CONTROLLER_BUTTON_DPAD_UP), DPAD_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_BACKWARD, makeInput(SDL_CONTROLLER_BUTTON_DPAD_DOWN), DPAD_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LATERAL_RIGHT, makeInput(SDL_CONTROLLER_BUTTON_DPAD_RIGHT), DPAD_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LATERAL_LEFT, makeInput(SDL_CONTROLLER_BUTTON_DPAD_LEFT), DPAD_MOVE_SPEED);
    
    // Button controls
    mapper.addInputChannel(UserInputMapper::VERTICAL_UP, makeInput(SDL_CONTROLLER_BUTTON_Y), DPAD_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::VERTICAL_DOWN, makeInput(SDL_CONTROLLER_BUTTON_X), DPAD_MOVE_SPEED);
    
    // Zoom
    mapper.addInputChannel(UserInputMapper::BOOM_IN, makeInput(RIGHT_SHOULDER), BOOM_SPEED);
    mapper.addInputChannel(UserInputMapper::BOOM_OUT, makeInput(LEFT_SHOULDER), BOOM_SPEED);
    
    
    // Hold front right shoulder button for precision controls
    // Left Joystick: Movement, strafing
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_FORWARD, makeInput(LEFT_AXIS_Y_NEG), makeInput(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER), JOYSTICK_MOVE_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_BACKWARD, makeInput(LEFT_AXIS_Y_POS), makeInput(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER), JOYSTICK_MOVE_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::LATERAL_RIGHT, makeInput(LEFT_AXIS_X_POS), makeInput(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER), JOYSTICK_MOVE_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::LATERAL_LEFT, makeInput(LEFT_AXIS_X_NEG), makeInput(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER), JOYSTICK_MOVE_SPEED/2.0f);
    
    // Right Joystick: Camera orientation
    mapper.addInputChannel(UserInputMapper::YAW_RIGHT, makeInput(RIGHT_AXIS_X_POS), makeInput(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER), JOYSTICK_YAW_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::YAW_LEFT, makeInput(RIGHT_AXIS_X_NEG), makeInput(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER), JOYSTICK_YAW_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::PITCH_UP, makeInput(RIGHT_AXIS_Y_NEG), makeInput(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER), JOYSTICK_PITCH_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::PITCH_DOWN, makeInput(RIGHT_AXIS_Y_POS), makeInput(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER), JOYSTICK_PITCH_SPEED/2.0f);
    
    // Dpad movement
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_FORWARD, makeInput(SDL_CONTROLLER_BUTTON_DPAD_UP), makeInput(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER), DPAD_MOVE_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_BACKWARD, makeInput(SDL_CONTROLLER_BUTTON_DPAD_DOWN), makeInput(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER), DPAD_MOVE_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::LATERAL_RIGHT, makeInput(SDL_CONTROLLER_BUTTON_DPAD_RIGHT), makeInput(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER), DPAD_MOVE_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::LATERAL_LEFT, makeInput(SDL_CONTROLLER_BUTTON_DPAD_LEFT), makeInput(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER), DPAD_MOVE_SPEED/2.0f);
    
    // Button controls
    mapper.addInputChannel(UserInputMapper::VERTICAL_UP, makeInput(SDL_CONTROLLER_BUTTON_Y), makeInput(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER), DPAD_MOVE_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::VERTICAL_DOWN, makeInput(SDL_CONTROLLER_BUTTON_X), makeInput(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER), DPAD_MOVE_SPEED/2.0f);
    
    // Zoom
    mapper.addInputChannel(UserInputMapper::BOOM_IN, makeInput(RIGHT_SHOULDER), makeInput(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER), BOOM_SPEED/2.0f);
    mapper.addInputChannel(UserInputMapper::BOOM_OUT, makeInput(LEFT_SHOULDER), makeInput(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER), BOOM_SPEED/2.0f);
    
    mapper.addInputChannel(UserInputMapper::SHIFT, makeInput(SDL_CONTROLLER_BUTTON_LEFTSHOULDER));
    
    mapper.addInputChannel(UserInputMapper::ACTION1, makeInput(SDL_CONTROLLER_BUTTON_B));
    mapper.addInputChannel(UserInputMapper::ACTION2, makeInput(SDL_CONTROLLER_BUTTON_A));
#endif
}

float Joystick::getButton(int channel) const {
    if (!_buttonPressedMap.empty()) {
        if (_buttonPressedMap.find(channel) != _buttonPressedMap.end()) {
            return 1.0f;
        } else {
            return 0.0f;
        }
    }
    return 0.0f;
}

float Joystick::getAxis(int channel) const {
    auto axis = _axisStateMap.find(channel);
    if (axis != _axisStateMap.end()) {
        return (*axis).second;
    } else {
        return 0.0f;
    }
}

#ifdef HAVE_SDL2
UserInputMapper::Input Joystick::makeInput(SDL_GameControllerButton button) {
    return UserInputMapper::Input(_deviceID, button, UserInputMapper::ChannelType::BUTTON);
}
#endif

UserInputMapper::Input Joystick::makeInput(Joystick::JoystickAxisChannel axis) {
    return UserInputMapper::Input(_deviceID, axis, UserInputMapper::ChannelType::AXIS);
}

