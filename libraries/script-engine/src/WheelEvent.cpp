//
//  WheelEvent.cpp
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

#include "WheelEvent.h"

WheelEvent::WheelEvent() :
    x(0.0f),
    y(0.0f),
    delta(0.0f),
    orientation("UNKNOwN"),
    isLeftButton(false),
    isRightButton(false),
    isMiddleButton(false),
    isShifted(false),
    isControl(false),
    isMeta(false),
    isAlt(false)
{
    
}

WheelEvent::WheelEvent(const QWheelEvent& event) {
    x = event.x();
    y = event.y();
    
    delta = event.delta();
    if (event.orientation() == Qt::Horizontal) {
        orientation = "HORIZONTAL";
    } else {
        orientation = "VERTICAL";
    }
    
    // button pressed state
    isLeftButton = (event.buttons().testFlag(Qt::LeftButton));
    isRightButton = (event.buttons().testFlag(Qt::RightButton));
    isMiddleButton = (event.buttons().testFlag(Qt::MiddleButton));
    
    // keyboard modifiers
    isShifted = event.modifiers().testFlag(Qt::ShiftModifier);
    isMeta = event.modifiers().testFlag(Qt::MetaModifier);
    isControl = event.modifiers().testFlag(Qt::ControlModifier);
    isAlt = event.modifiers().testFlag(Qt::AltModifier);
}


QScriptValue WheelEvent::toScriptValue(QScriptEngine* engine, const WheelEvent& event) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", event.x);
    obj.setProperty("y", event.y);
    obj.setProperty("delta", event.delta);
    obj.setProperty("orientation", event.orientation);
    obj.setProperty("isLeftButton", event.isLeftButton);
    obj.setProperty("isRightButton", event.isRightButton);
    obj.setProperty("isMiddleButton", event.isMiddleButton);
    obj.setProperty("isShifted", event.isShifted);
    obj.setProperty("isMeta", event.isMeta);
    obj.setProperty("isControl", event.isControl);
    obj.setProperty("isAlt", event.isAlt);
    return obj;
}

void WheelEvent::fromScriptValue(const QScriptValue& object, WheelEvent& event) {
    // nothing for now...
}