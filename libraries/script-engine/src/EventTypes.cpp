//
//  EventTypes.cpp
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 1/28/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDebug>
#include <RegisteredMetaTypes.h>

#include "KeyEvent.h"
#include "MouseEvent.h"
#include "TouchEvent.h"
#include "WheelEvent.h"

#include "EventTypes.h"

void registerEventTypes(QScriptEngine* engine) {
    qScriptRegisterMetaType(engine, KeyEvent::toScriptValue, KeyEvent::fromScriptValue);
    qScriptRegisterMetaType(engine, MouseEvent::toScriptValue, MouseEvent::fromScriptValue);
    qScriptRegisterMetaType(engine, TouchEvent::toScriptValue, TouchEvent::fromScriptValue);
    qScriptRegisterMetaType(engine, WheelEvent::toScriptValue, WheelEvent::fromScriptValue);
    qScriptRegisterMetaType(engine, spatialEventToScriptValue, spatialEventFromScriptValue);
}

SpatialEvent::SpatialEvent() : 
    locTranslation(0.0f), 
    locRotation(),
    absTranslation(0.0f), 
    absRotation()
{ 
}; 

SpatialEvent::SpatialEvent(const SpatialEvent& event) {
    locTranslation = event.locTranslation;
    locRotation = event.locRotation;
    absTranslation = event.absTranslation;
    absRotation = event.absRotation;
}


QScriptValue spatialEventToScriptValue(QScriptEngine* engine, const SpatialEvent& event) {
    QScriptValue obj = engine->newObject();

    obj.setProperty("locTranslation", vec3toScriptValue(engine, event.locTranslation) );
    obj.setProperty("locRotation", quatToScriptValue(engine, event.locRotation) );
    obj.setProperty("absTranslation", vec3toScriptValue(engine, event.absTranslation) );
    obj.setProperty("absRotation", quatToScriptValue(engine, event.absRotation) );

    return obj;
}

void spatialEventFromScriptValue(const QScriptValue& object,SpatialEvent& event) {
    // nothing for now...
}
