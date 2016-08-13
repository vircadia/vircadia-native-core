//
//  PointerEvent.cpp
//  script-engine/src
//
//  Created by Anthony Thibault on 2016-8-11.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <qscriptengine.h>
#include <qscriptvalue.h>

#include "PointerEvent.h"

static bool areFlagsSet(uint32_t flags, uint32_t mask) {
    return (flags & mask) != 0;
}

PointerEvent::PointerEvent() {
    ;
}

PointerEvent::PointerEvent(EventType type, uint32_t id,
             const glm::vec2& pos2D, const glm::vec3& pos3D,
             const glm::vec3& normal, const glm::vec3& direction,
             Button button, uint32_t buttons) :
    _type(type),
    _id(id),
    _pos2D(pos2D),
    _pos3D(pos3D),
    _normal(normal),
    _direction(direction),
    _button(button),
    _buttons(buttons)
{
    ;
}

QScriptValue PointerEvent::toScriptValue(QScriptEngine* engine, const PointerEvent& event) {
    QScriptValue obj = engine->newObject();

    switch (event._type) {
    case Press:
        obj.setProperty("type", "Press");
        break;
    case Release:
        obj.setProperty("type", "Release");
        break;
    default:
    case Move:
        obj.setProperty("type", "Move");
        break;
    };

    obj.setProperty("id", event._id);

    QScriptValue pos2D = engine->newObject();
    pos2D.setProperty("x", event._pos2D.x);
    pos2D.setProperty("y", event._pos2D.y);
    obj.setProperty("pos2D", pos2D);

    QScriptValue pos3D = engine->newObject();
    pos3D.setProperty("x", event._pos3D.x);
    pos3D.setProperty("y", event._pos3D.y);
    pos3D.setProperty("z", event._pos3D.z);
    obj.setProperty("pos3D", pos3D);

    QScriptValue normal = engine->newObject();
    normal.setProperty("x", event._normal.x);
    normal.setProperty("y", event._normal.y);
    normal.setProperty("z", event._normal.z);
    obj.setProperty("pos3D", normal);

    QScriptValue direction = engine->newObject();
    direction.setProperty("x", event._direction.x);
    direction.setProperty("y", event._direction.y);
    direction.setProperty("z", event._direction.z);
    obj.setProperty("pos3D", direction);

    switch (event._button) {
    case NoButtons:
        obj.setProperty("button", "None");
        break;
    case PrimaryButton:
        obj.setProperty("button", "Primary");
        break;
    case SecondaryButton:
        obj.setProperty("button", "Secondary");
        break;
    case TertiaryButton:
        obj.setProperty("button", "Tertiary");
        break;
    }

    obj.setProperty("isLeftButton", areFlagsSet(event._buttons, PrimaryButton));
    obj.setProperty("isRightButton", areFlagsSet(event._buttons, SecondaryButton));
    obj.setProperty("isMiddleButton", areFlagsSet(event._buttons, TertiaryButton));

    obj.setProperty("isPrimaryButton", areFlagsSet(event._buttons, PrimaryButton));
    obj.setProperty("isSecondaryButton", areFlagsSet(event._buttons, SecondaryButton));
    obj.setProperty("isTertiaryButton", areFlagsSet(event._buttons, TertiaryButton));

    return obj;
}

void PointerEvent::fromScriptValue(const QScriptValue& object, PointerEvent& event) {
    // nothing for now...
}
