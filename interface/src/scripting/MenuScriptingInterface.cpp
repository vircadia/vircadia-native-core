//
//  MenuScriptingInterface.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 2/25/14
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
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
    QMetaObject::invokeMethod(Menu::getInstance(), "addMenu", Q_ARG(const QString&, menu));
}

void MenuScriptingInterface::removeMenu(const QString& menu) {
    QMetaObject::invokeMethod(Menu::getInstance(), "removeMenu", Q_ARG(const QString&, menu));
}

void MenuScriptingInterface::addSeparator(const QString& menuName, const QString& separatorName) {
    QMetaObject::invokeMethod(Menu::getInstance(), "addSeparator",
                Q_ARG(const QString&, menuName),
                Q_ARG(const QString&, separatorName));
}

void MenuScriptingInterface::removeSeparator(const QString& menuName, const QString& separatorName) {
    QMetaObject::invokeMethod(Menu::getInstance(), "removeSeparator",
                Q_ARG(const QString&, menuName),
                Q_ARG(const QString&, separatorName));
}

void MenuScriptingInterface::addMenuItem(const MenuItemProperties& properties) {
    QMetaObject::invokeMethod(Menu::getInstance(), "addMenuItem", Q_ARG(const MenuItemProperties&, properties));
}

void MenuScriptingInterface::addMenuItem(const QString& menu, const QString& menuitem, const QString& shortcutKey) {
    MenuItemProperties properties(menu, menuitem, shortcutKey);
    QMetaObject::invokeMethod(Menu::getInstance(), "addMenuItem", Q_ARG(const MenuItemProperties&, properties));
}

void MenuScriptingInterface::addMenuItem(const QString& menu, const QString& menuitem) {
    MenuItemProperties properties(menu, menuitem);
    QMetaObject::invokeMethod(Menu::getInstance(), "addMenuItem", Q_ARG(const MenuItemProperties&, properties));
}

void MenuScriptingInterface::removeMenuItem(const QString& menu, const QString& menuitem) {
    QMetaObject::invokeMethod(Menu::getInstance(), "removeMenuItem", 
                Q_ARG(const QString&, menu),
                Q_ARG(const QString&, menuitem));
};

bool MenuScriptingInterface::isOptionChecked(const QString& menuOption) {
    bool result;
    QMetaObject::invokeMethod(Menu::getInstance(), "isOptionChecked", Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(bool, result), 
                Q_ARG(const QString&, menuOption));
    return result;
}

void MenuScriptingInterface::setIsOptionChecked(const QString& menuOption, bool isChecked) {
    QMetaObject::invokeMethod(Menu::getInstance(), "setIsOptionChecked", Qt::BlockingQueuedConnection,
                Q_ARG(const QString&, menuOption),
                Q_ARG(bool, isChecked));
}
