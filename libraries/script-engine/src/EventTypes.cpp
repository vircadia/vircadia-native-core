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
#include "EventTypes.h"


void registerEventTypes(QScriptEngine* engine) {
    qScriptRegisterMetaType(engine, keyEventToScriptValue, keyEventFromScriptValue);
    qScriptRegisterMetaType(engine, mouseEventToScriptValue, mouseEventFromScriptValue);
    qScriptRegisterMetaType(engine, touchEventToScriptValue, touchEventFromScriptValue);
    qScriptRegisterMetaType(engine, wheelEventToScriptValue, wheelEventFromScriptValue);
    qScriptRegisterMetaType(engine, spatialEventToScriptValue, spatialEventFromScriptValue);
}

KeyEvent::KeyEvent() :
    key(0),
    text(""),
    isShifted(false),
    isControl(false),
    isMeta(false),
    isAlt(false),
    isKeypad(false),
    isValid(false)
{
};


KeyEvent::KeyEvent(const QKeyEvent& event) {
    key = event.key();
    text = event.text();
    isShifted = event.modifiers().testFlag(Qt::ShiftModifier);
    isMeta = event.modifiers().testFlag(Qt::MetaModifier);
    isControl = event.modifiers().testFlag(Qt::ControlModifier);
    isAlt = event.modifiers().testFlag(Qt::AltModifier);
    isKeypad = event.modifiers().testFlag(Qt::KeypadModifier);
    isValid = true;

    // handle special text for special characters...
    if (key == Qt::Key_F1) {
        text = "F1";
    } else if (key == Qt::Key_F2) {
        text = "F2";
    } else if (key == Qt::Key_F3) {
        text = "F3";
    } else if (key == Qt::Key_F4) {
        text = "F4";
    } else if (key == Qt::Key_F5) {
        text = "F5";
    } else if (key == Qt::Key_F6) {
        text = "F6";
    } else if (key == Qt::Key_F7) {
        text = "F7";
    } else if (key == Qt::Key_F8) {
        text = "F8";
    } else if (key == Qt::Key_F9) {
        text = "F9";
    } else if (key == Qt::Key_F10) {
        text = "F10";
    } else if (key == Qt::Key_F11) {
        text = "F11";
    } else if (key == Qt::Key_F12) {
        text = "F12";
    } else if (key == Qt::Key_Up) {
        text = "UP";
    } else if (key == Qt::Key_Down) {
        text = "DOWN";
    } else if (key == Qt::Key_Left) {
        text = "LEFT";
    } else if (key == Qt::Key_Right) {
        text = "RIGHT";
    } else if (key == Qt::Key_Escape) {
        text = "ESC";
    } else if (key == Qt::Key_Tab) {
        text = "TAB";
    } else if (key == Qt::Key_Delete) {
        text = "DELETE";
    } else if (key == Qt::Key_Backspace) {
        text = "BACKSPACE";
    } else if (key == Qt::Key_Shift) {
        text = "SHIFT";
    } else if (key == Qt::Key_Alt) {
        text = "ALT";
    } else if (key == Qt::Key_Control) {
        text = "CONTROL";
    } else if (key == Qt::Key_Meta) {
        text = "META";
    } else if (key == Qt::Key_PageDown) {
        text = "PAGE DOWN";
    } else if (key == Qt::Key_PageUp) {
        text = "PAGE UP";
    } else if (key == Qt::Key_Home) {
        text = "HOME";
    } else if (key == Qt::Key_End) {
        text = "END";
    } else if (key == Qt::Key_Help) {
        text = "HELP";
    } else if (key == Qt::Key_CapsLock) {
        text = "CAPS LOCK";
    } else if (key >= Qt::Key_A && key <= Qt::Key_Z && (isMeta || isControl || isAlt))  {
        // this little bit of hackery will fix the text character keys like a-z in cases of control/alt/meta where
        // qt doesn't always give you the key characters and will sometimes give you crazy non-printable characters
        const int lowerCaseAdjust = 0x20;
        QString unicode;
        if (isShifted) {
            text = QString(QChar(key));
        } else {
            text = QString(QChar(key + lowerCaseAdjust));
        }
    }
}

bool KeyEvent::operator==(const KeyEvent& other) const { 
    return other.key == key 
        && other.isShifted == isShifted 
        && other.isControl == isControl
        && other.isMeta == isMeta
        && other.isAlt == isAlt
        && other.isKeypad == isKeypad; 
}


KeyEvent::operator QKeySequence() const { 
    int resultCode = 0;
    if (text.size() == 1 && text >= "a" && text <= "z") {
        resultCode = text.toUpper().at(0).unicode();
    } else {
        resultCode = key;
    }

    if (isMeta) {
        resultCode |= Qt::META;
    }
    if (isAlt) {
        resultCode |= Qt::ALT;
    }
    if (isControl) {
        resultCode |= Qt::CTRL;
    }
    if (isShifted) {
        resultCode |= Qt::SHIFT;
    }
    return QKeySequence(resultCode);
}

QScriptValue keyEventToScriptValue(QScriptEngine* engine, const KeyEvent& event) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("key", event.key);
    obj.setProperty("text", event.text);
    obj.setProperty("isShifted", event.isShifted);
    obj.setProperty("isMeta", event.isMeta);
    obj.setProperty("isControl", event.isControl);
    obj.setProperty("isAlt", event.isAlt);
    obj.setProperty("isKeypad", event.isKeypad);
    return obj;
}

void keyEventFromScriptValue(const QScriptValue& object, KeyEvent& event) {

    event.isValid = false; // assume the worst
    event.isMeta = object.property("isMeta").toVariant().toBool();
    event.isControl = object.property("isControl").toVariant().toBool();
    event.isAlt = object.property("isAlt").toVariant().toBool();
    event.isKeypad = object.property("isKeypad").toVariant().toBool();

    QScriptValue key = object.property("key");
    if (key.isValid()) {
        event.key = key.toVariant().toInt();
        event.text = QString(QChar(event.key));
        event.isValid = true;
    } else {
        QScriptValue text = object.property("text");
        if (text.isValid()) {
            event.text = object.property("text").toVariant().toString();
            
            // if the text is a special command, then map it here...
            // TODO: come up with more elegant solution here, a map? is there a Qt function that gives nice names for keys?
            if (event.text.toUpper() == "F1") {
                event.key = Qt::Key_F1;
            } else if (event.text.toUpper() == "F2") {
                event.key = Qt::Key_F2;
            } else if (event.text.toUpper() == "F3") {
                event.key = Qt::Key_F3;
            } else if (event.text.toUpper() == "F4") {
                event.key = Qt::Key_F4;
            } else if (event.text.toUpper() == "F5") {
                event.key = Qt::Key_F5;
            } else if (event.text.toUpper() == "F6") {
                event.key = Qt::Key_F6;
            } else if (event.text.toUpper() == "F7") {
                event.key = Qt::Key_F7;
            } else if (event.text.toUpper() == "F8") {
                event.key = Qt::Key_F8;
            } else if (event.text.toUpper() == "F9") {
                event.key = Qt::Key_F9;
            } else if (event.text.toUpper() == "F10") {
                event.key = Qt::Key_F10;
            } else if (event.text.toUpper() == "F11") {
                event.key = Qt::Key_F11;
            } else if (event.text.toUpper() == "F12") {
                event.key = Qt::Key_F12;
            } else if (event.text.toUpper() == "UP") {
                event.key = Qt::Key_Up;
                event.isKeypad = true;
            } else if (event.text.toUpper() == "DOWN") {
                event.key = Qt::Key_Down;
                event.isKeypad = true;
            } else if (event.text.toUpper() == "LEFT") {
                event.key = Qt::Key_Left;
                event.isKeypad = true;
            } else if (event.text.toUpper() == "RIGHT") {
                event.key = Qt::Key_Right;
                event.isKeypad = true;
            } else if (event.text.toUpper() == "ESC") {
                event.key = Qt::Key_Escape;
            } else if (event.text.toUpper() == "TAB") {
                event.key = Qt::Key_Tab;
            } else if (event.text.toUpper() == "DELETE") {
                event.key = Qt::Key_Delete;
            } else if (event.text.toUpper() == "BACKSPACE") {
                event.key = Qt::Key_Backspace;
            } else if (event.text.toUpper() == "SHIFT") {
                event.key = Qt::Key_Shift;
            } else if (event.text.toUpper() == "ALT") {
                event.key = Qt::Key_Alt;
            } else if (event.text.toUpper() == "CONTROL") {
                event.key = Qt::Key_Control;
            } else if (event.text.toUpper() == "META") {
                event.key = Qt::Key_Meta;
            } else if (event.text.toUpper() == "PAGE DOWN") {
                event.key = Qt::Key_PageDown;
            } else if (event.text.toUpper() == "PAGE UP") {
                event.key = Qt::Key_PageUp;
            } else if (event.text.toUpper() == "HOME") {
                event.key = Qt::Key_Home;
            } else if (event.text.toUpper() == "END") {
                event.key = Qt::Key_End;
            } else if (event.text.toUpper() == "HELP") {
                event.key = Qt::Key_Help;
            } else if (event.text.toUpper() == "CAPS LOCK") {
                event.key = Qt::Key_CapsLock;
            } else {
                event.key = event.text.at(0).unicode();
            }
            event.isValid = true;
        }
    }

    QScriptValue isShifted = object.property("isShifted");
    if (isShifted.isValid()) {
        event.isShifted = isShifted.toVariant().toBool();
    } else {
        // if no isShifted was included, get it from the text
        QChar character = event.text.at(0);
        if (character.isLetter() && character.isUpper()) {
            event.isShifted = true;
        } else {
            // if it's a symbol, then attempt to detect shifted-ness
            if (QString("~!@#$%^&*()_+{}|:\"<>?").contains(character)) {
                event.isShifted = true;
            }
        }
    }
    

    const bool wantDebug = false;
    if (wantDebug) {    
        qDebug() << "event.key=" << event.key
            << " event.text=" << event.text
            << " event.isShifted=" << event.isShifted
            << " event.isControl=" << event.isControl
            << " event.isMeta=" << event.isMeta
            << " event.isAlt=" << event.isAlt
            << " event.isKeypad=" << event.isKeypad;
    }
}

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
}; 


MouseEvent::MouseEvent(const QMouseEvent& event, const unsigned int deviceID) :
    x(event.x()), 
    y(event.y()),
    deviceID(deviceID),
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

QScriptValue mouseEventToScriptValue(QScriptEngine* engine, const MouseEvent& event) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", event.x);
    obj.setProperty("y", event.y);
    obj.setProperty("button", event.button);
    obj.setProperty("deviceID", event.deviceID);
    obj.setProperty("isLeftButton", event.isLeftButton);
    obj.setProperty("isRightButton", event.isRightButton);
    obj.setProperty("isMiddleButton", event.isMiddleButton);
    obj.setProperty("isShifted", event.isShifted);
    obj.setProperty("isMeta", event.isMeta);
    obj.setProperty("isControl", event.isControl);
    obj.setProperty("isAlt", event.isAlt);

    return obj;
}

void mouseEventFromScriptValue(const QScriptValue& object, MouseEvent& event) {
    // nothing for now...
}

TouchEvent::TouchEvent() : 
    x(0.0f), 
    y(0.0f),
    isPressed(false),
    isMoved(false),
    isStationary(false),
    isReleased(false),
    isShifted(false),
    isControl(false),
    isMeta(false),
    isAlt(false),
    touchPoints(0),
    points(),
    radius(0.0f),
    isPinching(false),
    isPinchOpening(false),
    angles(),
    angle(0.0f),
    deltaAngle(0.0f),
    isRotating(false),
    rotating("none")
{
};

TouchEvent::TouchEvent(const QTouchEvent& event) :
    // these values are not set by initWithQTouchEvent() because they only apply to comparing to other events
    isPinching(false),
    isPinchOpening(false),
    deltaAngle(0.0f),
    isRotating(false),
    rotating("none")
{
    initWithQTouchEvent(event);
}

TouchEvent::TouchEvent(const QTouchEvent& event, const TouchEvent& other) {
    initWithQTouchEvent(event);
    calculateMetaAttributes(other);
}

// returns the angle (in degrees) between two points (note: 0 degrees is 'east')
float angleBetweenPoints(const glm::vec2& a, const glm::vec2& b ) {
    glm::vec2 length = b - a;
    float angle = DEGREES_PER_RADIAN * std::atan2(length.y, length.x);
    if (angle < 0) {
        angle += 360.0f;
    };
    return angle;
}

void TouchEvent::initWithQTouchEvent(const QTouchEvent& event) {
    // convert the touch points into an average    
    const QList<QTouchEvent::TouchPoint>& tPoints = event.touchPoints();
    float touchAvgX = 0.0f;
    float touchAvgY = 0.0f;
    touchPoints = tPoints.count();
    if (touchPoints > 1) {
        for (int i = 0; i < touchPoints; ++i) {
            touchAvgX += tPoints[i].pos().x();
            touchAvgY += tPoints[i].pos().y();
            
            // add it to our points vector
            glm::vec2 thisPoint(tPoints[i].pos().x(), tPoints[i].pos().y());
            points << thisPoint;
        }
        touchAvgX /= (float)(touchPoints);
        touchAvgY /= (float)(touchPoints);
    } else {
        // I'm not sure this should ever happen, why would Qt send us a touch event for only one point?
        // maybe this happens in the case of a multi-touch where all but the last finger is released?
        touchAvgX = tPoints[0].pos().x();
        touchAvgY = tPoints[0].pos().y();
    }
    x = touchAvgX;
    y = touchAvgY;
    
    // after calculating the center point (average touch point), determine the maximum radius
    // also calculate the rotation angle for each point
    float maxRadius = 0.0f;
    glm::vec2 center(x,y);
    for (int i = 0; i < touchPoints; ++i) {
        glm::vec2 touchPoint(tPoints[i].pos().x(), tPoints[i].pos().y());
        float thisRadius = glm::distance(center,touchPoint);
        if (thisRadius > maxRadius) {
            maxRadius = thisRadius;
        }
        
        // calculate the angle for this point
        float thisAngle = angleBetweenPoints(center,touchPoint);
        angles << thisAngle;
    }
    radius = maxRadius;

    // after calculating the angles for each touch point, determine the average angle
    float totalAngle = 0.0f;
    for (int i = 0; i < touchPoints; ++i) {
        totalAngle += angles[i];
    }
    angle = totalAngle/(float)touchPoints;
    
    isPressed = event.touchPointStates().testFlag(Qt::TouchPointPressed);
    isMoved = event.touchPointStates().testFlag(Qt::TouchPointMoved);
    isStationary = event.touchPointStates().testFlag(Qt::TouchPointStationary);
    isReleased = event.touchPointStates().testFlag(Qt::TouchPointReleased);

    // keyboard modifiers
    isShifted = event.modifiers().testFlag(Qt::ShiftModifier);
    isMeta = event.modifiers().testFlag(Qt::MetaModifier);
    isControl = event.modifiers().testFlag(Qt::ControlModifier);
    isAlt = event.modifiers().testFlag(Qt::AltModifier);
}

void TouchEvent::calculateMetaAttributes(const TouchEvent& other) {
    // calculate comparative event attributes...
    if (other.radius > radius) {
        isPinching = true;
        isPinchOpening = false;
    } else if (other.radius < radius) {
        isPinchOpening = true;
        isPinching = false;
    } else {
        isPinching = other.isPinching;
        isPinchOpening = other.isPinchOpening;
    }

    // determine if the points are rotating...
    // note: if the number of touch points change between events, then we don't consider ourselves to be rotating
    if (touchPoints == other.touchPoints) {
        deltaAngle = angle - other.angle;
        if (other.angle < angle) {
            isRotating = true;
            rotating = "clockwise";
        } else if (other.angle > angle) {
            isRotating = true;
            rotating = "counterClockwise";
        } else {
            isRotating = false;
            rotating = "none";
        }
    } else {
        deltaAngle = 0.0f;
        isRotating = false;
        rotating = "none";
    }
}


QScriptValue touchEventToScriptValue(QScriptEngine* engine, const TouchEvent& event) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", event.x);
    obj.setProperty("y", event.y);
    obj.setProperty("isPressed", event.isPressed);
    obj.setProperty("isMoved", event.isMoved);
    obj.setProperty("isStationary", event.isStationary);
    obj.setProperty("isReleased", event.isReleased);
    obj.setProperty("isShifted", event.isShifted);
    obj.setProperty("isMeta", event.isMeta);
    obj.setProperty("isControl", event.isControl);
    obj.setProperty("isAlt", event.isAlt);
    obj.setProperty("touchPoints", event.touchPoints);
    
    QScriptValue pointsObj = engine->newArray();
    int index = 0;
    foreach (glm::vec2 point, event.points) {
        QScriptValue thisPoint = vec2toScriptValue(engine, point);
        pointsObj.setProperty(index, thisPoint);
        index++;
    }    
    obj.setProperty("points", pointsObj);
    obj.setProperty("radius", event.radius);
    obj.setProperty("isPinching", event.isPinching);
    obj.setProperty("isPinchOpening", event.isPinchOpening);

    obj.setProperty("angle", event.angle);
    obj.setProperty("deltaAngle", event.deltaAngle);
    QScriptValue anglesObj = engine->newArray();
    index = 0;
    foreach (float angle, event.angles) {
        anglesObj.setProperty(index, angle);
        index++;
    }    
    obj.setProperty("angles", anglesObj);

    obj.setProperty("isRotating", event.isRotating);
    obj.setProperty("rotating", event.rotating);
    return obj;
}

void touchEventFromScriptValue(const QScriptValue& object, TouchEvent& event) {
    // nothing for now...
}

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
}; 

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


QScriptValue wheelEventToScriptValue(QScriptEngine* engine, const WheelEvent& event) {
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

void wheelEventFromScriptValue(const QScriptValue& object, WheelEvent& event) {
    // nothing for now...
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
