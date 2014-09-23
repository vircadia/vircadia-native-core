//
//  JoystickInputController.cpp
//  interface/src/devices
//
//  Created by Stephen Birarda on 2014-09-23.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <qmetatype.h>

#include "JoystickInputController.h"

JoystickInputController::JoystickInputController() :
    _name(),
    _axes(0),
    _buttons(0)
{
    
}

JoystickInputController::JoystickInputController(const QString& name, int numAxes, int numButtons) :
    _name(name),
    _axes(numAxes),
    _buttons(numButtons)
{
    
}

JoystickInputController::JoystickInputController(const JoystickInputController& otherJoystickController) :
    _name(otherJoystickController._name),
    _axes(otherJoystickController._axes),
    _buttons(otherJoystickController._buttons)
{
    
}

JoystickInputController& JoystickInputController::operator=(const JoystickInputController& otherJoystickController) {
    JoystickInputController temp(otherJoystickController);
    swap(temp);
    return *this;
}

void JoystickInputController::swap(JoystickInputController& otherJoystickController) {
    using std::swap;
    
    swap(_name, otherJoystickController._name);
    swap(_axes, otherJoystickController._axes);
    swap(_buttons, otherJoystickController._buttons);
}
