//
//  MouseEvent.cpp
//  script-engine/src
//
//  Created by Stephen Birarda on 2014-10-27.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <qscriptengine.h>
#include <qscriptvalue.h>

#include "MouseEvent.h"

MouseEvent::MouseEvent() :
    x(0.0f),
    y(0.0f),
    isLeftButton(false),
    isRightButton(false),
    isMiddleButton(false),
    isShifted(false),
    isControl(false),
    isMeta(false),
    isAlt(false)
{
    
}


MouseEvent::MouseEvent(const QMouseEvent& event) :
    x(event.x()),
    y(event.y()),
    isLeftButton(event.buttons().testFlag(Qt::LeftButton)),
    isRightButton(event.buttons().testFlag(Qt::RightButton)),
    isMiddleButton(event.buttons().testFlag(Qt::MiddleButton)),
    isShifted(event.modifiers().testFlag(Qt::ShiftModifier)),
    isControl(event.modifiers().testFlag(Qt::ControlModifier)),
    isMeta(event.modifiers().testFlag(Qt::MetaModifier)),
    isAlt(event.modifiers().testFlag(Qt::AltModifier))
{
    // single button that caused the event
    switch (event.button()) {
        case Qt::LeftButton:
            button = "LEFT";
            isLeftButton = true;
            break;
        case Qt::RightButton:
            button = "RIGHT";
            isRightButton = true;
            break;
        case Qt::MiddleButton:
            button = "MIDDLE";
            isMiddleButton = true;
            break;
        default:
            button = "NONE";
            break;
    }
}

QScriptValue MouseEvent::toScriptValue(QScriptEngine* engine, const MouseEvent& event) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", event.x);
    obj.setProperty("y", event.y);
    obj.setProperty("button", event.button);
    obj.setProperty("isLeftButton", event.isLeftButton);
    obj.setProperty("isRightButton", event.isRightButton);
    obj.setProperty("isMiddleButton", event.isMiddleButton);
    obj.setProperty("isShifted", event.isShifted);
    obj.setProperty("isMeta", event.isMeta);
    obj.setProperty("isControl", event.isControl);
    obj.setProperty("isAlt", event.isAlt);
    
    return obj;
}

void MouseEvent::fromScriptValue(const QScriptValue& object, MouseEvent& event) {
    // nothing for now...
}