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

#include <PerfStat.h>

#include "JoystickManager.h"

using namespace std;

JoystickManager::JoystickManager() {
#ifdef HAVE_SDL
    SDL_Init(SDL_INIT_JOYSTICK);
    int joystickCount = SDL_NumJoysticks();
    for (int i = 0; i < joystickCount; i++) {
        SDL_Joystick* sdlJoystick = SDL_JoystickOpen(i);
        if (sdlJoystick) {
            _sdlJoysticks.append(sdlJoystick);
            
            JoystickInputController controller(SDL_JoystickName(i),
                                               SDL_JoystickNumAxes(sdlJoystick), SDL_JoystickNumButtons(sdlJoystick));
            _joysticks.append(controller);
        }
    }
#endif
}

JoystickManager::~JoystickManager() {
#ifdef HAVE_SDL
    foreach (SDL_Joystick* sdlJoystick, _sdlJoysticks) {
        SDL_JoystickClose(sdlJoystick);
    }
    SDL_Quit();
#endif
}

void JoystickManager::update() {
#ifdef HAVE_SDL
    PerformanceTimer perfTimer("joystick");
    SDL_JoystickUpdate();
    
    for (int i = 0; i < _joysticks.size(); i++) {
        SDL_Joystick* sdlJoystick = _sdlJoysticks.at(i);
        JoystickInputController& joystick = _joysticks[i];
        for (int j = 0; j < joystick.getNumAxes(); j++) {
            float value = glm::round(SDL_JoystickGetAxis(sdlJoystick, j) + 0.5f) / numeric_limits<short>::max();
            const float DEAD_ZONE = 0.1f;
            joystick.updateAxis(j, glm::abs(value) < DEAD_ZONE ? 0.0f : value);
        }
        for (int j = 0; j < joystick.getNumButtons(); j++) {
            joystick.updateButton(j, SDL_JoystickGetButton(sdlJoystick, j));
        }
    }
#endif
}
