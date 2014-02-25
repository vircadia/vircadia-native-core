//
//  MenuTypes.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/28/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  Used to register meta-types with Qt for very various event types so that they can be exposed to our
//  scripting engine

#include <QDebug>
#include <RegisteredMetaTypes.h>
#include "MenuTypes.h"


MenuItemProperties::MenuItemProperties() :
    menuName(""),
    menuItemName(""),
    shortcutKey(""),
    shortcutKeyEvent(),
    shortcutKeySequence(),
    position(-1),
    beforeItem(""),
    afterItem(""),
    isCheckable(false),
    isChecked(false)
{
};

MenuItemProperties::MenuItemProperties(const QString& menuName, const QString& menuItemName,
                        const QString& shortcutKey, bool checkable, bool checked) :
    menuName(menuName),
    menuItemName(menuItemName),
    shortcutKey(shortcutKey),
    shortcutKeyEvent(),
    shortcutKeySequence(shortcutKey),
    position(-1),
    beforeItem(""),
    afterItem(""),
    isCheckable(checkable),
    isChecked(checked)
{
}

MenuItemProperties::MenuItemProperties(const QString& menuName, const QString& menuItemName, 
                        const KeyEvent& shortcutKeyEvent, bool checkable, bool checked) :
    menuName(menuName),
    menuItemName(menuItemName),
    shortcutKey(""),
    shortcutKeyEvent(shortcutKeyEvent),
    shortcutKeySequence(shortcutKeyEvent),
    position(-1),
    beforeItem(""),
    afterItem(""),
    isCheckable(checkable),
    isChecked(checked)
{
}

void registerMenuTypes(QScriptEngine* engine) {
    qScriptRegisterMetaType(engine, menuItemPropertiesToScriptValue, menuItemPropertiesFromScriptValue);
}

QScriptValue menuItemPropertiesToScriptValue(QScriptEngine* engine, const MenuItemProperties& properties) {
    QScriptValue obj = engine->newObject();
    // not supported
    return obj;
}

void menuItemPropertiesFromScriptValue(const QScriptValue& object, MenuItemProperties& properties) {
    properties.menuName = object.property("menuName").toVariant().toString();
    properties.menuItemName = object.property("menuItemName").toVariant().toString();
    properties.isCheckable = object.property("isCheckable").toVariant().toBool();
    properties.isChecked = object.property("isChecked").toVariant().toBool();
    
    // handle the shortcut key options in order...
    QScriptValue shortcutKeyValue = object.property("shortcutKey");
    if (shortcutKeyValue.isValid()) {
        properties.shortcutKey = shortcutKeyValue.toVariant().toString();
        properties.shortcutKeySequence = properties.shortcutKey;
    } else {
        QScriptValue shortcutKeyEventValue = object.property("shortcutKeyEvent");
        if (shortcutKeyEventValue.isValid()) {
            keyEventFromScriptValue(shortcutKeyEventValue, properties.shortcutKeyEvent);
            properties.shortcutKeySequence = properties.shortcutKeyEvent;
        }
    }

    properties.position = object.property("position").toVariant().toInt();
    properties.beforeItem = object.property("beforeItem").toVariant().toString();
    properties.afterItem = object.property("afterItem").toVariant().toString();
}


