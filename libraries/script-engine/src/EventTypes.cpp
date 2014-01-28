//
//  EventTypes.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/28/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  Used to register meta-types with Qt for very various event types so that they can be exposed to our
//  scripting engine

#include "EventTypes.h"

void registerEventTypes(QScriptEngine* engine) {
    qScriptRegisterMetaType(engine, keyEventToScriptValue, keyEventFromScriptValue);
    qScriptRegisterMetaType(engine, mouseEventToScriptValue, mouseEventFromScriptValue);
    qScriptRegisterMetaType(engine, touchEventToScriptValue, touchEventFromScriptValue);
    qScriptRegisterMetaType(engine, wheelEventToScriptValue, wheelEventFromScriptValue);
}

QScriptValue keyEventToScriptValue(QScriptEngine* engine, const KeyEvent& event) {
    QScriptValue obj = engine->newObject();
    /*
    obj.setProperty("key", event.key());
    obj.setProperty("isAutoRepeat", event.isAutoRepeat());

    bool isShifted = event.modifiers().testFlag(Qt::ShiftModifier);
    bool isMeta = event.modifiers().testFlag(Qt::ControlModifier);

    obj.setProperty("isShifted", isShifted);
    obj.setProperty("isMeta", isMeta);
     */

    return obj;
}

void keyEventFromScriptValue(const QScriptValue &object, KeyEvent& event) {
    // nothing for now...
}

QScriptValue mouseEventToScriptValue(QScriptEngine* engine, const MouseEvent& event) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", event.x());
    obj.setProperty("y", event.y());
    return obj;
}

void mouseEventFromScriptValue(const QScriptValue &object, MouseEvent& event) {
    // nothing for now...
}

QScriptValue touchEventToScriptValue(QScriptEngine* engine, const TouchEvent& event) {
    QScriptValue obj = engine->newObject();

    // convert the touch points into an average    
    const QList<QTouchEvent::TouchPoint>& tPoints = event.touchPoints();
    float touchAvgX = 0.0f;
    float touchAvgY = 0.0f;
    int numTouches = tPoints.count();
    if (numTouches > 1) {
        for (int i = 0; i < numTouches; ++i) {
            touchAvgX += tPoints[i].pos().x();
            touchAvgY += tPoints[i].pos().y();
        }
        touchAvgX /= (float)(numTouches);
        touchAvgY /= (float)(numTouches);
    }
    
    obj.setProperty("averageX", touchAvgX);
    obj.setProperty("averageY", touchAvgY);
    return obj;
}

void touchEventFromScriptValue(const QScriptValue &object, TouchEvent& event) {
    // nothing for now...
}

QScriptValue wheelEventToScriptValue(QScriptEngine* engine, const WheelEvent& event) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", event.x());
    obj.setProperty("y", event.y());
    return obj;
}

void wheelEventFromScriptValue(const QScriptValue &object, WheelEvent& event) {
    // nothing for now...
}
