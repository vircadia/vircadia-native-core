//
//  HFActionEvent.cpp
//  interface/src/events
//
//  Created by Stephen Birarda on 2014-10-27.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HFActionEvent.h"

HFActionEvent::HFActionEvent(QEvent::Type type, const QPointF& localPosition) :
	HFMetaEvent(type),
    localPosition(localPosition)
{
    
}

QEvent::Type HFActionEvent::startType() {
    static QEvent::Type startType = HFMetaEvent::newEventType();
    return startType;
}

QEvent::Type HFActionEvent::endType() {
    static QEvent::Type endType = HFMetaEvent::newEventType();
    return endType;
}

QScriptValue HFActionEvent::toScriptValue(QScriptEngine* engine, const HFActionEvent& event) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", event.localPosition.x());
    obj.setProperty("y", event.localPosition.y());
    return obj;
}

void HFActionEvent::fromScriptValue(const QScriptValue& object, HFActionEvent& event) {
    // not yet implemented
}

