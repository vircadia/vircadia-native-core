//
//  EventTypes.h
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 1/28/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EventTypes_h
#define hifi_EventTypes_h

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QtScript/QScriptEngine>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QTouchEvent>
#include <QWheelEvent>


class KeyEvent {
public:
    KeyEvent();
    KeyEvent(const QKeyEvent& event);
    bool operator==(const KeyEvent& other) const;
    operator QKeySequence() const;

    int key;
    QString text;
    bool isShifted;
    bool isControl;
    bool isMeta;
    bool isAlt;
    bool isKeypad;
    bool isValid;
};


class MouseEvent {
public:
    MouseEvent(); 
    MouseEvent(const QMouseEvent& event);
    int x;
    int y;
    QString button;
    bool isLeftButton;
    bool isRightButton;
    bool isMiddleButton;
    bool isShifted;
    bool isControl;
    bool isMeta;
    bool isAlt;
};

class TouchEvent {
public:
    TouchEvent();
    TouchEvent(const QTouchEvent& event);
    TouchEvent(const QTouchEvent& event, const TouchEvent& other);

    float x;
    float y;
    bool isPressed;
    bool isMoved;
    bool isStationary;
    bool isReleased; 
    bool isShifted;
    bool isControl;
    bool isMeta;
    bool isAlt;
    int touchPoints;
    QVector<glm::vec2> points;
    float radius;
    bool isPinching;
    bool isPinchOpening;

    // angles are in degrees
    QVector<float> angles; // angle from center to each point
    float angle; // the average of the angles
    float deltaAngle; // the change in average angle from last event
    bool isRotating;
    QString rotating;

private:
    void initWithQTouchEvent(const QTouchEvent& event);
    void calculateMetaAttributes(const TouchEvent& other);
};

class WheelEvent {
public:
    WheelEvent();
    WheelEvent(const QWheelEvent& event);
    int x;
    int y;
    int delta;
    QString orientation;
    bool isLeftButton;
    bool isRightButton;
    bool isMiddleButton;
    bool isShifted;
    bool isControl;
    bool isMeta;
    bool isAlt;
};

class SpatialEvent {
public:
    SpatialEvent();
    SpatialEvent(const SpatialEvent& other);

    glm::vec3 locTranslation;
    glm::quat locRotation;
    glm::vec3 absTranslation;
    glm::quat absRotation;

private:
};


Q_DECLARE_METATYPE(KeyEvent)
Q_DECLARE_METATYPE(MouseEvent)
Q_DECLARE_METATYPE(TouchEvent)
Q_DECLARE_METATYPE(WheelEvent)
Q_DECLARE_METATYPE(SpatialEvent)

void registerEventTypes(QScriptEngine* engine);

QScriptValue keyEventToScriptValue(QScriptEngine* engine, const KeyEvent& event);
void keyEventFromScriptValue(const QScriptValue& object, KeyEvent& event);

QScriptValue mouseEventToScriptValue(QScriptEngine* engine, const MouseEvent& event);
void mouseEventFromScriptValue(const QScriptValue& object, MouseEvent& event);

QScriptValue touchEventToScriptValue(QScriptEngine* engine, const TouchEvent& event);
void touchEventFromScriptValue(const QScriptValue& object, TouchEvent& event);

QScriptValue wheelEventToScriptValue(QScriptEngine* engine, const WheelEvent& event);
void wheelEventFromScriptValue(const QScriptValue& object, WheelEvent& event);

QScriptValue spatialEventToScriptValue(QScriptEngine* engine, const SpatialEvent& event);
void spatialEventFromScriptValue(const QScriptValue& object, SpatialEvent& event);

#endif // hifi_EventTypes_h
