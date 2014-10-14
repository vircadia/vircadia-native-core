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

class Joystick : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(QString name READ getName)

#ifdef HAVE_SDL2
    Q_PROPERTY(int instanceId READ getInstanceId)
#endif
    
    Q_PROPERTY(int numAxes READ getNumAxes)
    Q_PROPERTY(int numButtons READ getNumButtons)
public:
    Joystick();
    ~Joystick();
    
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
    
    const QVector<float>& getAxes() const { return _axes; }
    const QVector<bool>& getButtons() const { return _buttons; }
    
    int getNumAxes() const { return _axes.size(); }
    int getNumButtons() const { return _buttons.size(); }
    
signals:
    void axisValueChanged(int axis, float newValue, float oldValue);
    void buttonStateChanged(int button, float newValue, float oldValue);
private:
#ifdef HAVE_SDL2
    SDL_GameController* _sdlGameController;
    SDL_Joystick* _sdlJoystick;
    SDL_JoystickID _instanceId;
#endif

    QString _name;
    QVector<float> _axes;
    QVector<bool> _buttons;
};

#endif // hifi_JoystickTracker_h
