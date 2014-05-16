//
//  JoystickManager.cpp
//  interface/src/devices
//
//  Created by Andrzej Kapolka on 5/15/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <limits>

#include <QtDebug>

#include <glm/glm.hpp>

#include "JoystickManager.h"

using namespace std;

JoystickManager::JoystickManager() {
#ifdef HAVE_SDL
    SDL_Init(SDL_INIT_JOYSTICK);
    int joystickCount = SDL_NumJoysticks();
    for (int i = 0; i < joystickCount; i++) {
        SDL_Joystick* joystick = SDL_JoystickOpen(i);
        if (joystick) {
            JoystickState state = { SDL_JoystickName(i), QVector<float>(SDL_JoystickNumAxes(joystick)),
                QVector<bool>(SDL_JoystickNumButtons(joystick)) };
            qDebug() << state.name << state.axes.size() << state.buttons.size();
            _joystickStates.append(state);
            _joysticks.append(joystick);
        }
    }
#endif
}

JoystickManager::~JoystickManager() {
#ifdef HAVE_SDL
    foreach (SDL_Joystick* joystick, _joysticks) {
        SDL_JoystickClose(joystick);
    }
    SDL_Quit();
#endif
}

void JoystickManager::update() {
#ifdef HAVE_SDL
    SDL_JoystickUpdate();
    
    for (int i = 0; i < _joystickStates.size(); i++) {
        SDL_Joystick* joystick = _joysticks.at(i);
        JoystickState& state = _joystickStates[i];
        for (int j = 0; j < state.axes.size(); j++) {
            state.axes[j] = glm::round(SDL_JoystickGetAxis(joystick, j) + 0.5f) / numeric_limits<short>::max();
        }
        for (int j = 0; j < state.buttons.size(); j++) {
            state.buttons[j] = SDL_JoystickGetButton(joystick, j);
        }
    }
#endif
}
