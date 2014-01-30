//
//  EventTypes.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/28/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  Used to register meta-types with Qt for very various event types so that they can be exposed to our
//  scripting engine

#include <QDebug>
#include "EventTypes.h"


KeyEvent::KeyEvent() {
    key = 0;
    text = QString("");
    isShifted = false;
    isMeta = false;
    isControl = false;
    isValid = false;
}


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
    }
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
    obj.setProperty("text", event.text);
    obj.setProperty("isShifted", event.isShifted);
    obj.setProperty("isMeta", event.isMeta);
    obj.setProperty("isControl", event.isControl);
    obj.setProperty("isAlt", event.isAlt);
    obj.setProperty("isKeypad", event.isKeypad);
    return obj;
}

void keyEventFromScriptValue(const QScriptValue &object, KeyEvent& event) {

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
