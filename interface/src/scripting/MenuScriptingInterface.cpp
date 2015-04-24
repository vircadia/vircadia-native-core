//
//  MenuScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Brad Hefta-Gaub on 2/25/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"

#include "MenuScriptingInterface.h"


MenuScriptingInterface* MenuScriptingInterface::getInstance() {
    static MenuScriptingInterface sharedInstance;
    return &sharedInstance;
}

void MenuScriptingInterface::menuItemTriggered() {
    QAction* menuItemAction = dynamic_cast<QAction*>(sender());
    if (menuItemAction) {
        // emit the event
        emit menuItemEvent(menuItemAction->text());
    }
}

void MenuScriptingInterface::addMenu(const QString& menu) {
    Menu::getInstance()->addMenu("", menu);
}

void MenuScriptingInterface::removeMenu(const QString& menu) {
    Menu::getInstance()->removeMenu(menu);
}

bool MenuScriptingInterface::menuExists(const QString& menu) {
    return Menu::getInstance()->menuExists(menu);
}

void MenuScriptingInterface::addSeparator(const QString& menuName, const QString& separatorName) {
    Menu::getInstance()->addSeparator(menuName, separatorName);
}

void MenuScriptingInterface::removeSeparator(const QString& menuName, const QString& separatorName) {
    Menu::getInstance()->removeSeparator(menuName, separatorName);
}

void MenuScriptingInterface::addMenuItem(const MenuItemProperties& properties) {
    Menu::getInstance()->addMenuItem(properties);
}

void MenuScriptingInterface::addMenuItem(const QString& menu, const QString& menuitem, const QString& shortcutKey) {
    Menu::getInstance()->addItem(menu, menuitem);
    Menu::getInstance()->setItemShortcut(menuitem, shortcutKey);
}

void MenuScriptingInterface::addMenuItem(const QString& menu, const QString& menuitem) {
    Menu::getInstance()->addItem(menu, menuitem);
}

void MenuScriptingInterface::removeMenuItem(const QString& menu, const QString& menuitem) {
    Menu::getInstance()->removeItem(menuitem);
};

bool MenuScriptingInterface::menuItemExists(const QString& menu, const QString& menuitem) {
    return Menu::getInstance()->itemExists(menu, menuitem);
}

bool MenuScriptingInterface::isOptionChecked(const QString& menuOption) {
    return Menu::getInstance()->isChecked(menuOption);
}

void MenuScriptingInterface::setIsOptionChecked(const QString& menuOption, bool isChecked) {
    Menu::getInstance()->setChecked(menuOption, isChecked);
}
