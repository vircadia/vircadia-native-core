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

#include <QtDebug>

#ifdef HAVE_SDL
#include <SDL.h>
#undef main
#endif

#include <PerfStat.h>

#include "JoystickScriptingInterface.h"

JoystickScriptingInterface& JoystickScriptingInterface::getInstance() {
    static JoystickScriptingInterface sharedInstance;
    return sharedInstance;
}

JoystickScriptingInterface::JoystickScriptingInterface() :
    _openJoysticks(),
    _availableDeviceNames(),
    _isInitialized(false)
{
    reset();
}

JoystickScriptingInterface::~JoystickScriptingInterface() {
    qDeleteAll(_openJoysticks);
    
#ifdef HAVE_SDL
    SDL_Quit();
    _isInitialized = false;
#endif
}

void JoystickScriptingInterface::reset() {
#ifdef HAVE_SDL
    
    if (_isInitialized) {
        _isInitialized = false;
        
        // close all the open joysticks before we quit
        foreach(Joystick* openJoystick, _openJoysticks) {
            openJoystick->closeJoystick();
        }
        
        SDL_Quit();
    }
    
    bool initSuccess = (SDL_Init(SDL_INIT_JOYSTICK) == 0);
    
    if (initSuccess) {
        
        int joystickCount = SDL_NumJoysticks();
        
        for (int i = 0; i < joystickCount; i++) {
            _availableDeviceNames << SDL_JoystickName(i);
        }
        
        foreach(const QString& joystickName, _openJoysticks.keys()) {
            _openJoysticks[joystickName]->setSDLJoystick(openSDLJoystickWithName(joystickName));
        }
        
        _isInitialized = true;
    }
#endif
}

void JoystickScriptingInterface::update() {
#ifdef HAVE_SDL
    if (_isInitialized) {
        PerformanceTimer perfTimer("JoystickScriptingInterface::update");
        SDL_JoystickUpdate();
        
        foreach(Joystick* joystick, _openJoysticks) {
            joystick->update();
        }
    }
#endif
}

Joystick* JoystickScriptingInterface::joystickWithName(const QString& name) {
    Joystick* matchingJoystick = _openJoysticks.value(name);
#ifdef HAVE_SDL
    if (!matchingJoystick) {
        SDL_Joystick* openSDLJoystick = openSDLJoystickWithName(name);
        
        if (openSDLJoystick) {
            matchingJoystick = _openJoysticks.insert(name, new Joystick(name, openSDLJoystick)).value();
        } else {
            qDebug() << "No matching joystick found with name" << name << "- returning NULL pointer.";
        }
    }
#endif
    
    return matchingJoystick;
}


#ifdef HAVE_SDL

SDL_Joystick* JoystickScriptingInterface::openSDLJoystickWithName(const QString &name) {
    // we haven't opened a joystick with this name yet - enumerate our SDL devices and see if it exists
    int joystickCount = SDL_NumJoysticks();
    
    for (int i = 0; i < joystickCount; i++) {
        if (SDL_JoystickName(i) == name) {
            return SDL_JoystickOpen(i);
            break;
        }
    }
    
    return NULL;
}

#endif
