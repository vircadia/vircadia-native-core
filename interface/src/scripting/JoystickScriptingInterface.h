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

#include "devices/Joystick.h"

/// Handles joystick input through SDL.
class JoystickScriptingInterface : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(QStringList availableJoystickNames READ getAvailableJoystickNames)
public:
    static JoystickScriptingInterface& getInstance();
    
    const QStringList& getAvailableJoystickNames() const { return _availableDeviceNames; }
    
    void update();
    
public slots:
    Joystick* joystickWithName(const QString& name);
    
private:
    JoystickScriptingInterface();
    ~JoystickScriptingInterface();
    
    QMap<QString, Joystick*> _openJoysticks;
    QStringList _availableDeviceNames;
};

#endif // hifi_JoystickScriptingInterface_h
