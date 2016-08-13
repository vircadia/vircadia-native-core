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

#include "HFActionEvent.h"
#include "KeyEvent.h"
#include "MouseEvent.h"
#include "SpatialEvent.h"
#include "PointerEvent.h"
#include "TouchEvent.h"
#include "WheelEvent.h"

#include "EventTypes.h"

void registerEventTypes(QScriptEngine* engine) {
    qScriptRegisterMetaType(engine, HFActionEvent::toScriptValue, HFActionEvent::fromScriptValue);
    qScriptRegisterMetaType(engine, KeyEvent::toScriptValue, KeyEvent::fromScriptValue);
    qScriptRegisterMetaType(engine, MouseEvent::toScriptValue, MouseEvent::fromScriptValue);
    qScriptRegisterMetaType(engine, PointerEvent::toScriptValue, PointerEvent::fromScriptValue);
    qScriptRegisterMetaType(engine, TouchEvent::toScriptValue, TouchEvent::fromScriptValue);
    qScriptRegisterMetaType(engine, WheelEvent::toScriptValue, WheelEvent::fromScriptValue);
    qScriptRegisterMetaType(engine, SpatialEvent::toScriptValue, SpatialEvent::fromScriptValue);
}
