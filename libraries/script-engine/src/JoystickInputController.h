//
//  JoystickInputController.h
//  interface/src/devices
//
//  Created by Stephen Birarda on 2014-09-23.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_JoystickInputController_h
#define hifi_JoystickInputController_h

#include <qobject.h>
#include <qvector.h>

class JoystickInputController : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(QString name READ getName)
    
    Q_PROPERTY(int numAxes READ getNumAxes)
    Q_PROPERTY(QVector<float> axes READ getAxes)
public:
    JoystickInputController();
    JoystickInputController(const QString& name, int numAxes, int numButtons);
    JoystickInputController(const JoystickInputController& otherJoystickController);
    JoystickInputController& operator=(const JoystickInputController& otherJoystickController);
    
    const QString& getName() const { return _name; }
    
    void updateAxis(int index, float value) { _axes[index] = value; }
    void updateButton(int index, bool isActive) { _buttons[index] = isActive; }
    
    const QVector<float>& getAxes() const { return _axes; }
    const QVector<bool>& getButtons() const { return _buttons; }
    
    int getNumAxes() const { return _axes.size(); }
    int getNumButtons() const { return _buttons.size(); }
    
private:
    void swap(JoystickInputController& otherJoystickController);
    
    QString _name;
    QVector<float> _axes;
    QVector<bool> _buttons;
};

#endif // hifi_JoystickTracker_h