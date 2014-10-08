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

#ifdef HAVE_SDL
#include <SDL.h>
#undef main
#endif

class Joystick : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(QString name READ getName)
    
    Q_PROPERTY(int numAxes READ getNumAxes)
    Q_PROPERTY(int numButtons READ getNumButtons)
public:
    Joystick();
    ~Joystick();
    
#ifdef HAVE_SDL
    Joystick(const QString& name, SDL_Joystick* sdlJoystick);
#endif
    
    void update();
    
    void closeJoystick();
    void setSDLJoystick(SDL_Joystick* sdlJoystick) { _sdlJoystick = sdlJoystick; }
    
    const QString& getName() const { return _name; }
    
    const QVector<float>& getAxes() const { return _axes; }
    const QVector<bool>& getButtons() const { return _buttons; }
    
    int getNumAxes() const { return _axes.size(); }
    int getNumButtons() const { return _buttons.size(); }
    
signals:
    void axisValueChanged(int axis, float newValue, float oldValue);
    void buttonStateChanged(int button, float newValue, float oldValue);
private:
    QString _name;
    QVector<float> _axes;
    QVector<bool> _buttons;
    
#ifdef HAVE_SDL
    SDL_Joystick* _sdlJoystick;
#endif
};

#endif // hifi_JoystickTracker_h