//
//  SDL2Manager.cpp
//  interface/src/devices
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

#include "Application.h"

#include "SDL2Manager.h"

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
#ifdef HAVE_SDL2
    bool initSuccess = (SDL_Init(SDL_INIT_GAMECONTROLLER) == 0);
    
    if (initSuccess) {
        int joystickCount = SDL_NumJoysticks();
        
        for (int i = 0; i < joystickCount; i++) {
            SDL_GameController* controller = SDL_GameControllerOpen(i);
            
            if (controller) {
                SDL_JoystickID id = getInstanceId(controller);
                if (!_openJoysticks.contains(id)) {
                    Joystick* joystick = new Joystick(id, SDL_GameControllerName(controller), controller);
                    _openJoysticks[id] = joystick;
                    joystick->registerToUserInputMapper(*Application::getUserInputMapper());
                    joystick->assignDefaultInputMapping(*Application::getUserInputMapper());
                    emit joystickAdded(joystick);
                }
            }
        }
        
        _isInitialized = true;
    } else {
        qDebug() << "Error initializing SDL2 Manager";
    }
#endif
}

SDL2Manager::~SDL2Manager() {
#ifdef HAVE_SDL2
    qDeleteAll(_openJoysticks);
    
    SDL_Quit();
#endif
}

SDL2Manager* SDL2Manager::getInstance() {
    static SDL2Manager sharedInstance;
    return &sharedInstance;
}

void SDL2Manager::focusOutEvent() {
#ifdef HAVE_SDL2
    for (auto joystick : _openJoysticks) {
        joystick->focusOutEvent();
    }
#endif
}

void SDL2Manager::update() {
#ifdef HAVE_SDL2
    if (_isInitialized) {
        for (auto joystick : _openJoysticks) {
            joystick->update();
        }
        
        PerformanceTimer perfTimer("SDL2Manager::update");
        SDL_GameControllerUpdate();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_CONTROLLERAXISMOTION) {
                Joystick* joystick = _openJoysticks[event.caxis.which];
                if (joystick) {
                    joystick->handleAxisEvent(event.caxis);
                }
            } else if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
                Joystick* joystick = _openJoysticks[event.cbutton.which];
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
                } else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_A) {
                    // this will either start or stop a global action event
                    QEvent::Type actionType = (event.type == SDL_CONTROLLERBUTTONDOWN)
                    ? HFActionEvent::startType()
                    : HFActionEvent::endType();
                    
                    // global action events fire in the center of the screen
                    Application* app = Application::getInstance();
                    PickRay pickRay = app->getCamera()->computePickRay(app->getTrueMouseX(),
                                                                       app->getTrueMouseY());
                    HFActionEvent actionEvent(actionType, pickRay);
                    qApp->sendEvent(qApp, &actionEvent);
                }
                
            } else if (event.type == SDL_CONTROLLERDEVICEADDED) {
                SDL_GameController* controller = SDL_GameControllerOpen(event.cdevice.which);
                
                SDL_JoystickID id = getInstanceId(controller);
                if (!_openJoysticks.contains(id)) {
                    Joystick* joystick = new Joystick(id, SDL_GameControllerName(controller), controller);
                    _openJoysticks[id] = joystick;
                    joystick->registerToUserInputMapper(*Application::getUserInputMapper());
                    joystick->assignDefaultInputMapping(*Application::getUserInputMapper());
                    emit joystickAdded(joystick);
                }
            } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
                Joystick* joystick = _openJoysticks[event.cdevice.which];
                _openJoysticks.remove(event.cdevice.which);
                Application::getUserInputMapper()->removeDevice(joystick->getDeviceID());
                emit joystickRemoved(joystick);
            }
        }
    }
#endif
}
