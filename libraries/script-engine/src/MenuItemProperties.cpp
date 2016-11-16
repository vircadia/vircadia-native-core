//
//  MenuItemProperties.cpp
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
#include "MenuItemProperties.h"



MenuItemProperties::MenuItemProperties(const QString& menuName, const QString& menuItemName,
                        const QString& shortcutKey, bool checkable, bool checked, bool separator) :
    menuName(menuName),
    menuItemName(menuItemName),
    shortcutKey(shortcutKey),
    shortcutKeyEvent(),
    shortcutKeySequence(shortcutKey),
    isCheckable(checkable),
    isChecked(checked),
    isSeparator(separator)
{
}

MenuItemProperties::MenuItemProperties(const QString& menuName, const QString& menuItemName,
                        const KeyEvent& shortcutKeyEvent, bool checkable, bool checked, bool separator) :
    menuName(menuName),
    menuItemName(menuItemName),
    shortcutKeyEvent(shortcutKeyEvent),
    shortcutKeySequence(shortcutKeyEvent),
    isCheckable(checkable),
    isChecked(checked),
    isSeparator(separator)
{
}

void registerMenuItemProperties(QScriptEngine* engine) {
    qScriptRegisterMetaType(engine, menuItemPropertiesToScriptValue, menuItemPropertiesFromScriptValue);
}

QScriptValue menuItemPropertiesToScriptValue(QScriptEngine* engine, const MenuItemProperties& properties) {
    QScriptValue obj = engine->newObject();
    // not supported
    return obj;
}

/**jsdoc
 * `MenuItemProperties` is a list of properties that can be passed to Menu.addMenuItem
 * to create a new menu item.
 *
 * If none of position, beforeItem, afterItem, or grouping are specified, the
 * menu item will be placed in the last position.
 *
 * @typedef {Object} Menu.MenuItemProperties
 * @property {string} menuName Name of the top-level menu
 * @property {string} menuItemName Name of the menu item
 * @property {bool} isCheckable Whether the menu item is checkable or not
 * @property {bool} isChecked Where the menu item is checked or not
 * @property {string} shortcutKey An optional shortcut key to trigger the menu item.
 * @property {int} position The position to place the new menu item. `0` is the first menu item.
 * @property {string} beforeItem The name of the menu item to place this menu item before.
 * @property {string} afterItem The name of the menu item to place this menu item after.
 * @property {string} grouping The name of grouping to add this menu item to.
 */
void menuItemPropertiesFromScriptValue(const QScriptValue& object, MenuItemProperties& properties) {
    properties.menuName = object.property("menuName").toVariant().toString();
    properties.menuItemName = object.property("menuItemName").toVariant().toString();
    properties.isCheckable = object.property("isCheckable").toVariant().toBool();
    properties.isChecked = object.property("isChecked").toVariant().toBool();
    properties.isSeparator = object.property("isSeparator").toVariant().toBool();

    // handle the shortcut key options in order...
    QScriptValue shortcutKeyValue = object.property("shortcutKey");
    if (shortcutKeyValue.isValid()) {
        properties.shortcutKey = shortcutKeyValue.toVariant().toString();
        properties.shortcutKeySequence = properties.shortcutKey;
    } else {
        QScriptValue shortcutKeyEventValue = object.property("shortcutKeyEvent");
        if (shortcutKeyEventValue.isValid()) {
            KeyEvent::fromScriptValue(shortcutKeyEventValue, properties.shortcutKeyEvent);
            properties.shortcutKeySequence = properties.shortcutKeyEvent;
        }
    }

    if (object.property("position").isValid()) {
        properties.position = object.property("position").toVariant().toInt();
    }
    properties.beforeItem = object.property("beforeItem").toVariant().toString();
    properties.afterItem = object.property("afterItem").toVariant().toString();
    properties.grouping = object.property("grouping").toVariant().toString();
}


