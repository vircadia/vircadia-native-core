//
//  MenuScriptingInterface.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 2/25/14
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include "Application.h"

#include "MenuScriptingInterface.h"


MenuScriptingInterface* MenuScriptingInterface::_instance = NULL;
QMutex MenuScriptingInterface::_instanceMutex;

MenuScriptingInterface* MenuScriptingInterface::getInstance() {
    // lock the menu instance mutex to make sure we don't race and create two menus and crash
    _instanceMutex.lock();
    if (!_instance) {
        _instance = new MenuScriptingInterface();
    }
    _instanceMutex.unlock();
    return _instance;
}

void MenuScriptingInterface::deleteLaterIfExists() {
    _instanceMutex.lock();
    if (_instance) {
        _instance->deleteLater();
        _instance = NULL;
    }
    _instanceMutex.unlock();
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
    return Menu::getInstance()->isOptionChecked(menuOption);
}

void MenuScriptingInterface::setIsOptionChecked(const QString& menuOption, bool isChecked) {
    return Menu::getInstance()->setIsOptionChecked(menuOption, isChecked);
}
