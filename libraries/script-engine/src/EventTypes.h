//
//  EventTypes.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/28/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi_EventTypes_h__
#define __hifi_EventTypes_h__

#include <glm/glm.hpp>

#include <QtScript/QScriptEngine>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QTouchEvent>
#include <QWheelEvent>

class KeyEvent {
public:
    KeyEvent() { }; 
    KeyEvent(QKeyEvent* other) { }; 
};

class MouseEvent {
public:
    MouseEvent() { };
};

class TouchEvent {
public:
    TouchEvent() { };
};

class WheelEvent {
public:
    WheelEvent() { };
};


Q_DECLARE_METATYPE(KeyEvent)
Q_DECLARE_METATYPE(MouseEvent)
Q_DECLARE_METATYPE(TouchEvent)
Q_DECLARE_METATYPE(WheelEvent)

void registerEventTypes(QScriptEngine* engine);

QScriptValue keyEventToScriptValue(QScriptEngine* engine, const KeyEvent& event);
void keyEventFromScriptValue(const QScriptValue &object, KeyEvent& event);

QScriptValue mouseEventToScriptValue(QScriptEngine* engine, const MouseEvent& event);
void mouseEventFromScriptValue(const QScriptValue &object, MouseEvent& event);

QScriptValue touchEventToScriptValue(QScriptEngine* engine, const TouchEvent& event);
void touchEventFromScriptValue(const QScriptValue &object, TouchEvent& event);

QScriptValue wheelEventToScriptValue(QScriptEngine* engine, const WheelEvent& event);
void wheelEventFromScriptValue(const QScriptValue &object, WheelEvent& event);

#endif // __hifi_EventTypes_h__
