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

#ifdef HAVE_SDL

Joystick::Joystick(const QString& name, SDL_Joystick* sdlJoystick) :
    _name(name),
    _axes(QVector<float>(SDL_JoystickNumAxes(sdlJoystick))),
    _buttons(QVector<bool>(SDL_JoystickNumButtons(sdlJoystick))),
    _sdlJoystick(sdlJoystick)
{
    
}

#endif

Joystick::~Joystick() {
    closeJoystick();
}

void Joystick::closeJoystick() {
#ifdef HAVE_SDL
    SDL_JoystickClose(_sdlJoystick);
#endif
}

void Joystick::update() {
#ifdef HAVE_SDL
    // update our current values, emit a signal when there is a change
    for (int j = 0; j < getNumAxes(); j++) {
        float value = glm::round(SDL_JoystickGetAxis(_sdlJoystick, j) + 0.5f) / std::numeric_limits<short>::max();
        const float DEAD_ZONE = 0.1f;
        float cleanValue = glm::abs(value) < DEAD_ZONE ? 0.0f : value;
        
        if (_axes[j] != cleanValue) {
            float oldValue =  _axes[j];
            _axes[j] = cleanValue;
            emit axisValueChanged(j, cleanValue, oldValue);
        }
    }
    for (int j = 0; j < getNumButtons(); j++) {
        bool newValue = SDL_JoystickGetButton(_sdlJoystick, j);
        if (_buttons[j] != newValue) {
            bool oldValue = _buttons[j];
            _buttons[j] = newValue;
            emit buttonStateChanged(j, newValue, oldValue);
        }
    }
#endif
}