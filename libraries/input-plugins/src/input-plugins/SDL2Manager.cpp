//
//  SDL2Manager.cpp
//  input-plugins/src/input-plugins
//
//  Created by Sam Gondelman on 6/5/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <qapplication.h>

#include <HFActionEvent.h>
#include <HFBackEvent.h>
#include <PerfStat.h>
#include <controllers/UserInputMapper.h>

#include "SDL2Manager.h"

const QString SDL2Manager::NAME = "SDL2";

#ifdef HAVE_SDL2
SDL_JoystickID SDL2Manager::getInstanceId(SDL_GameController* controller) {
    SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller);
    return SDL_JoystickInstanceID(joystick);
}
#endif

SDL2Manager::SDL2Manager() :
#ifdef HAVE_SDL2
_openJoysticks(),
#endif
_isInitialized(false)
{
}

void SDL2Manager::init() {
#ifdef HAVE_SDL2
    bool initSuccess = (SDL_Init(SDL_INIT_GAMECONTROLLER) == 0);

    if (initSuccess) {
        int joystickCount = SDL_NumJoysticks();

        for (int i = 0; i < joystickCount; i++) {
            SDL_GameController* controller = SDL_GameControllerOpen(i);

            if (controller) {
                SDL_JoystickID id = getInstanceId(controller);
                if (!_openJoysticks.contains(id)) {
                    //Joystick* joystick = new Joystick(id, SDL_GameControllerName(controller), controller);
                    Joystick::Pointer joystick  = std::make_shared<Joystick>(id, controller);
                    _openJoysticks[id] = joystick;
                    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
                    userInputMapper->registerDevice(joystick);
                    emit joystickAdded(joystick.get());
                }
            }
        }

        _isInitialized = true;
    }
    else {
        qDebug() << "Error initializing SDL2 Manager";
    }
#endif
}

void SDL2Manager::deinit() {
#ifdef HAVE_SDL2
    _openJoysticks.clear();

    SDL_Quit();
#endif
}

void SDL2Manager::activate() {
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    for (auto joystick : _openJoysticks) {
        userInputMapper->registerDevice(joystick);
        emit joystickAdded(joystick.get());
    }
}

void SDL2Manager::deactivate() {
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    for (auto joystick : _openJoysticks) {
        userInputMapper->removeDevice(joystick->getDeviceID());
        emit joystickRemoved(joystick.get());
    }
}


bool SDL2Manager::isSupported() const {
#ifdef HAVE_SDL2
    return true;
#else
    return false;
#endif
}

void SDL2Manager::pluginFocusOutEvent() {
#ifdef HAVE_SDL2
    for (auto joystick : _openJoysticks) {
        joystick->focusOutEvent();
    }
#endif
}

void SDL2Manager::pluginUpdate(float deltaTime, bool jointsCaptured) {
#ifdef HAVE_SDL2
    if (_isInitialized) {
        auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
        for (auto joystick : _openJoysticks) {
            joystick->update(deltaTime, jointsCaptured);
        }
        
        PerformanceTimer perfTimer("SDL2Manager::update");
        SDL_GameControllerUpdate();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_CONTROLLERAXISMOTION) {
                Joystick::Pointer joystick = _openJoysticks[event.caxis.which];
                if (joystick) {
                    joystick->handleAxisEvent(event.caxis);
                }
            } else if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
                Joystick::Pointer joystick = _openJoysticks[event.cbutton.which];
                if (joystick) {
                    joystick->handleButtonEvent(event.cbutton);
                }
                
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_BACK) {
                    // this will either start or stop a global back event
                    QEvent::Type backType = (event.type == SDL_CONTROLLERBUTTONDOWN)
                    ? HFBackEvent::startType()
                    : HFBackEvent::endType();
                    HFBackEvent backEvent(backType);
                    
                    qApp->sendEvent(qApp, &backEvent);
                }
                
            } else if (event.type == SDL_CONTROLLERDEVICEADDED) {
                SDL_GameController* controller = SDL_GameControllerOpen(event.cdevice.which);
                SDL_JoystickID id = getInstanceId(controller);
                if (!_openJoysticks.contains(id)) {
                    // Joystick* joystick = new Joystick(id, SDL_GameControllerName(controller), controller);
                    Joystick::Pointer joystick = std::make_shared<Joystick>(id, controller);
                    _openJoysticks[id] = joystick;
                    userInputMapper->registerDevice(joystick);
                    emit joystickAdded(joystick.get());
                }
            } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
                if (_openJoysticks.contains(event.cdevice.which)) {
                    Joystick::Pointer joystick = _openJoysticks[event.cdevice.which];
                    _openJoysticks.remove(event.cdevice.which);
                    userInputMapper->removeDevice(joystick->getDeviceID());
                    emit joystickRemoved(joystick.get());
                }
            }
        }
    }
#endif
}
