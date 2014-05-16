//
//  JoystickManager.h
//  interface/src/devices
//
//  Created by Andrzej Kapolka on 5/15/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_JoystickManager_h
#define hifi_JoystickManager_h

#include <QObject>
#include <QVector>

#ifdef HAVE_SDL
#include <SDL.h>
#undef main
#endif

class JoystickState;

/// Handles joystick input through SDL.
class JoystickManager : public QObject {
    Q_OBJECT

public:
    
    JoystickManager();
    virtual ~JoystickManager();
    
    const QVector<JoystickState>& getJoystickStates() const { return _joystickStates; }
    
    void update();

private:
    QVector<JoystickState> _joystickStates;
    
#ifdef HAVE_SDL
    QVector<SDL_Joystick*> _joysticks;
#endif
};

class JoystickState {
public:
    QString name;
    QVector<float> axes;
    QVector<bool> buttons;
};

#endif // hifi_JoystickManager_h
