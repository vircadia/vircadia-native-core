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

#include "Joystick.h"


#ifdef HAVE_SDL2
const float MAX_AXIS = 32768.0f;

Joystick::Joystick(SDL_JoystickID instanceId, const QString& name, SDL_GameController* sdlGameController) :
    _sdlGameController(sdlGameController),
    _sdlJoystick(SDL_GameControllerGetJoystick(_sdlGameController)),
    _instanceId(instanceId),
    _name(name),
    _axes(QVector<float>(SDL_JoystickNumAxes(_sdlJoystick))),
    _buttons(QVector<bool>(SDL_JoystickNumButtons(_sdlJoystick)))
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


#ifdef HAVE_SDL2
void Joystick::handleAxisEvent(const SDL_ControllerAxisEvent& event) {
    if (event.axis < _axes.size() - 1) {
        float oldValue = _axes[event.axis];
        float newValue = event.value / MAX_AXIS;
        _axes[event.axis] = newValue;
        emit axisValueChanged(event.axis, newValue, oldValue);
    }
}

void Joystick::handleButtonEvent(const SDL_ControllerButtonEvent& event) {
    bool oldValue = _buttons[event.button];
    bool newValue = event.state == SDL_PRESSED;
    _buttons[event.button] = newValue;
    emit buttonStateChanged(event.button, newValue, oldValue);
}
#endif
