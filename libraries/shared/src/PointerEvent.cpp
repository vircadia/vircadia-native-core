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

#include "PointerEvent.h"

#include <qscriptengine.h>
#include <qscriptvalue.h>

#include "RegisteredMetaTypes.h"

static bool areFlagsSet(uint32_t flags, uint32_t mask) {
    return (flags & mask) != 0;
}

PointerEvent::PointerEvent() {
    ;
}

PointerEvent::PointerEvent(EventType type, uint32_t id,
                           const glm::vec2& pos2D, const glm::vec3& pos3D,
                           const glm::vec3& normal, const glm::vec3& direction,
                           Button button, uint32_t buttons, Qt::KeyboardModifiers keyboardModifiers) :
    _type(type),
    _id(id),
    _pos2D(pos2D),
    _pos3D(pos3D),
    _normal(normal),
    _direction(direction),
    _button(button),
    _buttons(buttons),
    _keyboardModifiers(keyboardModifiers)
{
    ;
}

QScriptValue PointerEvent::toScriptValue(QScriptEngine* engine, const PointerEvent& event) {
    QScriptValue obj = engine->newObject();

    switch (event._type) {
    case Press:
        obj.setProperty("type", "Press");
        break;
    case DoublePress:
        obj.setProperty("type", "DoublePress");
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

    bool isPrimaryButton = false;
    bool isSecondaryButton = false;
    bool isTertiaryButton = false;
    switch (event._button) {
    case NoButtons:
        obj.setProperty("button", "None");
        break;
    case PrimaryButton:
        obj.setProperty("button", "Primary");
        isPrimaryButton = true;
        break;
    case SecondaryButton:
        obj.setProperty("button", "Secondary");
        isSecondaryButton = true;
        break;
    case TertiaryButton:
        obj.setProperty("button", "Tertiary");
        isTertiaryButton = true;
        break;
    }

    if (isPrimaryButton) {
        obj.setProperty("isPrimaryButton", isPrimaryButton);
        obj.setProperty("isLeftButton", isPrimaryButton);
    }
    if (isSecondaryButton) {
        obj.setProperty("isSecondaryButton", isSecondaryButton);
        obj.setProperty("isRightButton", isSecondaryButton);
    }
    if (isTertiaryButton) {
        obj.setProperty("isTertiaryButton", isTertiaryButton);
        obj.setProperty("isMiddleButton", isTertiaryButton);
    }

    obj.setProperty("isPrimaryHeld", areFlagsSet(event._buttons, PrimaryButton));
    obj.setProperty("isSecondaryHeld", areFlagsSet(event._buttons, SecondaryButton));
    obj.setProperty("isTertiaryHeld", areFlagsSet(event._buttons, TertiaryButton));

    obj.setProperty("keyboardModifiers", QScriptValue(event.getKeyboardModifiers()));

    return obj;
}

void PointerEvent::fromScriptValue(const QScriptValue& object, PointerEvent& event) {
    if (object.isObject()) {
        QScriptValue type = object.property("type");
        QString typeStr = type.isString() ? type.toString() : "Move";
        if (typeStr == "Press") {
            event._type = Press;
        } else if (typeStr == "DoublePress") {
            event._type = DoublePress;
        } else if (typeStr == "Release") {
            event._type = Release;
        } else {
            event._type = Move;
        }

        QScriptValue id = object.property("id");
        event._id = id.isNumber() ? (uint32_t)id.toNumber() : 0;

        glm::vec2 pos2D;
        vec2FromScriptValue(object.property("pos2D"), event._pos2D);

        glm::vec3 pos3D;
        vec3FromScriptValue(object.property("pos3D"), event._pos3D);

        glm::vec3 normal;
        vec3FromScriptValue(object.property("normal"), event._normal);

        glm::vec3 direction;
        vec3FromScriptValue(object.property("direction"), event._direction);

        QScriptValue button = object.property("button");
        QString buttonStr = type.isString() ? button.toString() : "NoButtons";

        if (buttonStr == "Primary") {
            event._button = PrimaryButton;
        } else if (buttonStr == "Secondary") {
            event._button = SecondaryButton;
        } else if (buttonStr == "Tertiary") {
            event._button = TertiaryButton;
        } else {
            event._button = NoButtons;
        }

        bool primary = object.property("isPrimaryHeld").toBool();
        bool secondary = object.property("isSecondaryHeld").toBool();
        bool tertiary = object.property("isTertiaryHeld").toBool();
        event._buttons = 0;
        if (primary) {
            event._buttons |= PrimaryButton;
        }
        if (secondary) {
            event._buttons |= SecondaryButton;
        }
        if (tertiary) {
            event._buttons |= TertiaryButton;
        }

        event._keyboardModifiers = (Qt::KeyboardModifiers)(object.property("keyboardModifiers").toUInt32());
    }
}

static const char* typeToStringMap[PointerEvent::NumEventTypes] = { "Press", "DoublePress", "Release", "Move" };
static const char* buttonsToStringMap[8] = {
    "NoButtons",
    "PrimaryButton",
    "SecondaryButton",
    "PrimaryButton | SecondaryButton",
    "TertiaryButton",
    "PrimaryButton | TertiaryButton",
    "SecondaryButton | TertiaryButton",
    "PrimaryButton | SecondaryButton | TertiaryButton",
};

QDebug& operator<<(QDebug& dbg, const PointerEvent& p) {
    dbg.nospace() << "PointerEvent, type = " << typeToStringMap[p.getType()] << ", id = " << p.getID();
    dbg.nospace() << ", pos2D = (" << p.getPos2D().x << ", " << p.getPos2D().y;
    dbg.nospace() << "), pos3D = (" << p.getPos3D().x << ", " << p.getPos3D().y << ", " << p.getPos3D().z;
    dbg.nospace() << "), normal = (" << p.getNormal().x << ", " << p.getNormal().y << ", " << p.getNormal().z;
    dbg.nospace() << "), dir = (" << p.getDirection().x << ", " << p.getDirection().y << ", " << p.getDirection().z;
    dbg.nospace() << "), button = " << buttonsToStringMap[p.getButton()] << " " << (int)p.getButton();
    dbg.nospace() << ", buttons = " << buttonsToStringMap[p.getButtons()];
    return dbg;
}
