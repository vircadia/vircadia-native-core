//
//  Joystick.h
//  interface/src/devices
//
//  Created by Stephen Birarda on 2014-09-23.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Joystick_h
#define hifi_Joystick_h

#include <qobject.h>
#include <qvector.h>

#ifdef HAVE_SDL2
#include <SDL.h>
#undef main
#endif

#include "ui/UserInputMapper.h"

class Joystick : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(QString name READ getName)

#ifdef HAVE_SDL2
    Q_PROPERTY(int instanceId READ getInstanceId)
#endif
    
public:
    enum JoystickAxisChannel {
        LEFT_AXIS_X_POS = 0,
        LEFT_AXIS_X_NEG,
        LEFT_AXIS_Y_POS,
        LEFT_AXIS_Y_NEG,
        RIGHT_AXIS_X_POS,
        RIGHT_AXIS_X_NEG,
        RIGHT_AXIS_Y_POS,
        RIGHT_AXIS_Y_NEG,
        RIGHT_SHOULDER,
        LEFT_SHOULDER,
    };
    
    Joystick();
    ~Joystick();
    
    typedef std::unordered_set<int> ButtonPressedMap;
    typedef std::map<int, float> AxisStateMap;
    
    float getButton(int channel) const;
    float getAxis(int channel) const;
    
#ifdef HAVE_SDL2
    UserInputMapper::Input makeInput(SDL_GameControllerButton button);
#endif
    UserInputMapper::Input makeInput(Joystick::JoystickAxisChannel axis);
    
    void registerToUserInputMapper(UserInputMapper& mapper);
    void assignDefaultInputMapping(UserInputMapper& mapper);
    
    void update();
    void focusOutEvent();
    
#ifdef HAVE_SDL2
    Joystick(SDL_JoystickID instanceId, const QString& name, SDL_GameController* sdlGameController);
#endif
    
    void closeJoystick();

#ifdef HAVE_SDL2
    void handleAxisEvent(const SDL_ControllerAxisEvent& event);
    void handleButtonEvent(const SDL_ControllerButtonEvent& event);
#endif
    
    const QString& getName() const { return _name; }
#ifdef HAVE_SDL2
    int getInstanceId() const { return _instanceId; }
#endif
    
    int getDeviceID() { return _deviceID; }
    
private:
#ifdef HAVE_SDL2
    SDL_GameController* _sdlGameController;
    SDL_Joystick* _sdlJoystick;
    SDL_JoystickID _instanceId;
#endif

    QString _name;
    
protected:
    int _deviceID = 0;
    
    ButtonPressedMap _buttonPressedMap;
    AxisStateMap _axisStateMap;
};

#endif // hifi_Joystick_h
