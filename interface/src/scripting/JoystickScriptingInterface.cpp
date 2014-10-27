//
//  JoystickScriptingInterface.cpp
//  interface/src/devices
//
//  Created by Andrzej Kapolka on 5/15/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <qapplication.h>
#include <QtDebug>
#include <QScriptValue>

#ifdef HAVE_SDL2
#include <SDL.h>
#undef main
#endif

#include <HFBackEvent.h>
#include <PerfStat.h>

#include "JoystickScriptingInterface.h"

#ifdef HAVE_SDL2
SDL_JoystickID getInstanceId(SDL_GameController* controller) {
    SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller);
    return SDL_JoystickInstanceID(joystick);
}
#endif

JoystickScriptingInterface& JoystickScriptingInterface::getInstance() {
    static JoystickScriptingInterface sharedInstance;
    return sharedInstance;
}

JoystickScriptingInterface::JoystickScriptingInterface() :
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
            SDL_JoystickID id = getInstanceId(controller);
            Joystick* joystick = new Joystick(id, SDL_GameControllerName(controller), controller);
            _openJoysticks[id] = joystick;
        }

        _isInitialized = true;
    } else {
        qDebug() << "Error initializing SDL";
    }
#endif
}

JoystickScriptingInterface::~JoystickScriptingInterface() {
#ifdef HAVE_SDL2
    qDeleteAll(_openJoysticks);
    
    SDL_Quit();
#endif
}

const QObjectList JoystickScriptingInterface::getAllJoysticks() const {
    QObjectList objectList;
#ifdef HAVE_SDL2
    const QList<Joystick*> joystickList = _openJoysticks.values();
    for (int i = 0; i < joystickList.length(); i++) {
        objectList << joystickList[i];
    }
#endif
    return objectList;
}

Joystick* JoystickScriptingInterface::joystickWithName(const QString& name) {
#ifdef HAVE_SDL2
    QMap<SDL_JoystickID, Joystick*>::iterator iter = _openJoysticks.begin();
    while (iter != _openJoysticks.end()) {
        if (iter.value()->getName() == name) {
            return iter.value();
        }
        iter++;
    }
#endif
    return NULL;
}

void JoystickScriptingInterface::update() {
#ifdef HAVE_SDL2
    if (_isInitialized) {
        PerformanceTimer perfTimer("JoystickScriptingInterface::update");
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
                    // this will either start or stop a cancel event
                    QEvent::Type backType = (event.type == SDL_CONTROLLERBUTTONDOWN)
                        ? HFBackEvent::startType()
                        : HFBackEvent::endType();
                    HFBackEvent backEvent(backType);
                    
                    qApp->sendEvent(qApp, &backEvent);
                }
                
            } else if (event.type == SDL_CONTROLLERDEVICEADDED) {
                SDL_GameController* controller = SDL_GameControllerOpen(event.cdevice.which);

                SDL_JoystickID id = getInstanceId(controller);
                Joystick* joystick = new Joystick(id, SDL_GameControllerName(controller), controller);
                _openJoysticks[id] = joystick;
                emit joystickAdded(joystick);
            } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
                Joystick* joystick = _openJoysticks[event.cdevice.which];
                _openJoysticks.remove(event.cdevice.which);
                emit joystickRemoved(joystick);
            }
        }
    }
#endif
}
