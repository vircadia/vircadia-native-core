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


KeyEvent::KeyEvent() {
    key = 0;
    isShifted = false;
    isMeta = false;
    isValid = false;
}


KeyEvent::KeyEvent(const QKeyEvent& event) {
    key = event.key();
    isShifted = event.modifiers().testFlag(Qt::ShiftModifier);
    isMeta = event.modifiers().testFlag(Qt::ControlModifier);
    isValid = true;
}

MouseEvent::MouseEvent(const QMouseEvent& event) {
    x = event.x();
    y = event.y();
}

TouchEvent::TouchEvent(const QTouchEvent& event) {
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
    x = touchAvgX;
    y = touchAvgY;
}

WheelEvent::WheelEvent(const QWheelEvent& event) {
    x = event.x();
    y = event.y();
}


void registerEventTypes(QScriptEngine* engine) {
    qScriptRegisterMetaType(engine, keyEventToScriptValue, keyEventFromScriptValue);
    qScriptRegisterMetaType(engine, mouseEventToScriptValue, mouseEventFromScriptValue);
    qScriptRegisterMetaType(engine, touchEventToScriptValue, touchEventFromScriptValue);
    qScriptRegisterMetaType(engine, wheelEventToScriptValue, wheelEventFromScriptValue);
}

QScriptValue keyEventToScriptValue(QScriptEngine* engine, const KeyEvent& event) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("key", event.key);
    obj.setProperty("isShifted", event.isShifted);
    obj.setProperty("isMeta", event.isMeta);
    return obj;
}

void keyEventFromScriptValue(const QScriptValue &object, KeyEvent& event) {
    event.key = object.property("key").toVariant().toInt();
    event.isShifted = object.property("isShifted").toVariant().toBool();
    event.isMeta = object.property("isMeta").toVariant().toBool();
    event.isValid = object.property("key").isValid();
}

QScriptValue mouseEventToScriptValue(QScriptEngine* engine, const MouseEvent& event) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", event.x);
    obj.setProperty("y", event.y);
    return obj;
}

void mouseEventFromScriptValue(const QScriptValue &object, MouseEvent& event) {
    // nothing for now...
}

QScriptValue touchEventToScriptValue(QScriptEngine* engine, const TouchEvent& event) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", event.x);
    obj.setProperty("y", event.y);
    return obj;
}

void touchEventFromScriptValue(const QScriptValue &object, TouchEvent& event) {
    // nothing for now...
}

QScriptValue wheelEventToScriptValue(QScriptEngine* engine, const WheelEvent& event) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", event.x);
    obj.setProperty("y", event.y);
    return obj;
}

void wheelEventFromScriptValue(const QScriptValue &object, WheelEvent& event) {
    // nothing for now...
}
