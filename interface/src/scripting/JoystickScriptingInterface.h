//
//  JoystickScriptingInterface.h
//  interface/src/devices
//
//  Created by Andrzej Kapolka on 5/15/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_JoystickScriptingInterface_h
#define hifi_JoystickScriptingInterface_h

#include <QObject>
#include <QVector>

#ifdef HAVE_SDL2
#include <SDL.h>
#endif

#include "devices/Joystick.h"

/// Handles joystick input through SDL.
class JoystickScriptingInterface : public QObject {
    Q_OBJECT
    
#ifdef HAVE_SDL2
    Q_PROPERTY(int AXIS_INVALID READ axisInvalid)
    Q_PROPERTY(int AXIS_LEFT_X READ axisLeftX)
    Q_PROPERTY(int AXIS_LEFT_Y READ axisLeftY)
    Q_PROPERTY(int AXIS_RIGHT_X READ axisRightX)
    Q_PROPERTY(int AXIS_RIGHT_Y READ axisRightY)
    Q_PROPERTY(int AXIS_TRIGGER_LEFT READ axisTriggerLeft)
    Q_PROPERTY(int AXIS_TRIGGER_RIGHT READ axisTriggerRight)
    Q_PROPERTY(int AXIS_MAX READ axisMax)

    Q_PROPERTY(int BUTTON_INVALID READ buttonInvalid)
    Q_PROPERTY(int BUTTON_FACE_BOTTOM READ buttonFaceBottom)
    Q_PROPERTY(int BUTTON_FACE_RIGHT READ buttonFaceRight)
    Q_PROPERTY(int BUTTON_FACE_LEFT READ buttonFaceLeft)
    Q_PROPERTY(int BUTTON_FACE_TOP READ buttonFaceTop)
    Q_PROPERTY(int BUTTON_BACK READ buttonBack)
    Q_PROPERTY(int BUTTON_GUIDE READ buttonGuide)
    Q_PROPERTY(int BUTTON_START READ buttonStart)
    Q_PROPERTY(int BUTTON_LEFT_STICK READ buttonLeftStick)
    Q_PROPERTY(int BUTTON_RIGHT_STICK READ buttonRightStick)
    Q_PROPERTY(int BUTTON_LEFT_SHOULDER READ buttonLeftShoulder)
    Q_PROPERTY(int BUTTON_RIGHT_SHOULDER READ buttonRightShoulder)
    Q_PROPERTY(int BUTTON_DPAD_UP READ buttonDpadUp)
    Q_PROPERTY(int BUTTON_DPAD_DOWN READ buttonDpadDown)
    Q_PROPERTY(int BUTTON_DPAD_LEFT READ buttonDpadLeft)
    Q_PROPERTY(int BUTTON_DPAD_RIGHT READ buttonDpadRight)
    Q_PROPERTY(int BUTTON_MAX READ buttonMax)

    Q_PROPERTY(int BUTTON_PRESSED READ buttonPressed)
    Q_PROPERTY(int BUTTON_RELEASED READ buttonRelease)
#endif

public:
    static JoystickScriptingInterface& getInstance();
    
    void update();
    
public slots:
    const QObjectList getAllJoysticks() const;

signals:
    void joystickAdded(Joystick* joystick);
    void joystickRemoved(Joystick* joystick);
    
private:
#ifdef HAVE_SDL2
    int axisInvalid() const { return SDL_CONTROLLER_AXIS_INVALID; }
    int axisLeftX() const { return SDL_CONTROLLER_AXIS_LEFTX; }
    int axisLeftY() const { return SDL_CONTROLLER_AXIS_LEFTY; }
    int axisRightX() const { return SDL_CONTROLLER_AXIS_RIGHTX; }
    int axisRightY() const { return SDL_CONTROLLER_AXIS_RIGHTY; }
    int axisTriggerLeft() const { return SDL_CONTROLLER_AXIS_TRIGGERLEFT; }
    int axisTriggerRight() const { return SDL_CONTROLLER_AXIS_TRIGGERRIGHT; }
    int axisMax() const { return SDL_CONTROLLER_AXIS_MAX; }

    int buttonInvalid() const { return SDL_CONTROLLER_BUTTON_INVALID; }
    int buttonFaceBottom() const { return SDL_CONTROLLER_BUTTON_A; }
    int buttonFaceRight() const { return SDL_CONTROLLER_BUTTON_B; }
    int buttonFaceLeft() const { return SDL_CONTROLLER_BUTTON_X; }
    int buttonFaceTop() const { return SDL_CONTROLLER_BUTTON_Y; }
    int buttonBack() const { return SDL_CONTROLLER_BUTTON_BACK; }
    int buttonGuide() const { return SDL_CONTROLLER_BUTTON_GUIDE; }
    int buttonStart() const { return SDL_CONTROLLER_BUTTON_START; }
    int buttonLeftStick() const { return SDL_CONTROLLER_BUTTON_LEFTSTICK; }
    int buttonRightStick() const { return SDL_CONTROLLER_BUTTON_RIGHTSTICK; }
    int buttonLeftShoulder() const { return SDL_CONTROLLER_BUTTON_LEFTSHOULDER; }
    int buttonRightShoulder() const { return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER; }
    int buttonDpadUp() const { return SDL_CONTROLLER_BUTTON_DPAD_UP; }
    int buttonDpadDown() const { return SDL_CONTROLLER_BUTTON_DPAD_DOWN; }
    int buttonDpadLeft() const { return SDL_CONTROLLER_BUTTON_DPAD_LEFT; }
    int buttonDpadRight() const { return SDL_CONTROLLER_BUTTON_DPAD_RIGHT; }
    int buttonMax() const { return SDL_CONTROLLER_BUTTON_MAX; }

    int buttonPressed() const { return SDL_PRESSED; }
    int buttonRelease() const { return SDL_RELEASED; }

    SDL_Joystick* openSDLJoystickWithName(const QString& name);
#endif
    
    JoystickScriptingInterface();
    ~JoystickScriptingInterface();
    
#ifdef HAVE_SDL2
    QMap<SDL_JoystickID, Joystick*> _openJoysticks;
#endif
    bool _isInitialized;
};

#endif // hifi_JoystickScriptingInterface_h
