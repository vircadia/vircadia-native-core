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

#include "MenuScriptingInterface.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>

#include <shared/QtHelpers.h>
#include <MenuItemProperties.h>
#include "Menu.h"

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

void MenuScriptingInterface::addMenu(const QString& menu, const QString& grouping) {
    Menu* menuInstance = Menu::getInstance();
    if (!menuInstance) {
        return;
    }

    QMetaObject::invokeMethod(menuInstance, "addMenu", Q_ARG(const QString&, menu), Q_ARG(const QString&, grouping));
}

void MenuScriptingInterface::removeMenu(const QString& menu) {
    Menu* menuInstance = Menu::getInstance();
    if (!menuInstance) {
        return;
    }

    QMetaObject::invokeMethod(menuInstance, "removeMenu", Q_ARG(const QString&, menu));
}

bool MenuScriptingInterface::menuExists(const QString& menu) {
    Menu* menuInstance = Menu::getInstance();
    if (!menuInstance) {
        return false;
    }

    if (QThread::currentThread() == qApp->thread()) {
        return menuInstance && menuInstance->menuExists(menu);
    }

    bool result { false };

    BLOCKING_INVOKE_METHOD(menuInstance, "menuExists",
                Q_RETURN_ARG(bool, result), 
                Q_ARG(const QString&, menu));

    return result;
}

void MenuScriptingInterface::addSeparator(const QString& menuName, const QString& separatorName) {
    Menu* menuInstance = Menu::getInstance();
    if (!menuInstance) {
        return;
    }

    QMetaObject::invokeMethod(menuInstance, "addSeparator",
                Q_ARG(const QString&, menuName),
                Q_ARG(const QString&, separatorName));
}

void MenuScriptingInterface::removeSeparator(const QString& menuName, const QString& separatorName) {
    Menu* menuInstance = Menu::getInstance();
    if (!menuInstance) {
        return;
    }

    QMetaObject::invokeMethod(menuInstance, "removeSeparator",
                Q_ARG(const QString&, menuName),
                Q_ARG(const QString&, separatorName));
}

void MenuScriptingInterface::addMenuItem(const MenuItemProperties& properties) {
    Menu* menuInstance = Menu::getInstance();
    if (!menuInstance) {
        return;
    }

    QMetaObject::invokeMethod(menuInstance, "addMenuItem", Q_ARG(const MenuItemProperties&, properties));
}

void MenuScriptingInterface::addMenuItem(const QString& menu, const QString& menuitem, const QString& shortcutKey) {
    Menu* menuInstance = Menu::getInstance();
    if (!menuInstance) {
        return;
    }

    MenuItemProperties properties(menu, menuitem, shortcutKey);
    QMetaObject::invokeMethod(menuInstance, "addMenuItem", Q_ARG(const MenuItemProperties&, properties));
}

void MenuScriptingInterface::addMenuItem(const QString& menu, const QString& menuitem) {
    Menu* menuInstance = Menu::getInstance();
    if (!menuInstance) {
        return;
    }

    MenuItemProperties properties(menu, menuitem);
    QMetaObject::invokeMethod(menuInstance, "addMenuItem", Q_ARG(const MenuItemProperties&, properties));
}

void MenuScriptingInterface::removeMenuItem(const QString& menu, const QString& menuitem) {
    Menu* menuInstance = Menu::getInstance();
    if (!menuInstance) {
        return;
    }
    QMetaObject::invokeMethod(menuInstance, "removeMenuItem",
                Q_ARG(const QString&, menu),
                Q_ARG(const QString&, menuitem));
};

bool MenuScriptingInterface::menuItemExists(const QString& menu, const QString& menuitem) {
    Menu* menuInstance = Menu::getInstance();
    if (!menuInstance) {
        return false;
    }

    if (QThread::currentThread() == qApp->thread()) {
        return menuInstance && menuInstance->menuItemExists(menu, menuitem);
    }

    bool result { false };

    BLOCKING_INVOKE_METHOD(menuInstance, "menuItemExists",
        Q_RETURN_ARG(bool, result),
        Q_ARG(const QString&, menu),
        Q_ARG(const QString&, menuitem));

    return result;
}

bool MenuScriptingInterface::isOptionChecked(const QString& menuOption) {
    Menu* menuInstance = Menu::getInstance();
    if (!menuInstance) {
        return false;
    }

    if (QThread::currentThread() == qApp->thread()) {
        return menuInstance && menuInstance->isOptionChecked(menuOption);
    }

    bool result { false };

    BLOCKING_INVOKE_METHOD(menuInstance, "isOptionChecked",
                Q_RETURN_ARG(bool, result), 
                Q_ARG(const QString&, menuOption));
    return result;
}

void MenuScriptingInterface::setIsOptionChecked(const QString& menuOption, bool isChecked) {
    Menu* menuInstance = Menu::getInstance();
    if (!menuInstance) {
        return;
    }

    QMetaObject::invokeMethod(menuInstance, "setIsOptionChecked",
                Q_ARG(const QString&, menuOption),
                Q_ARG(bool, isChecked));
}

bool MenuScriptingInterface::isMenuEnabled(const QString& menuOption) {
    Menu* menuInstance = Menu::getInstance();
    if (!menuInstance) {
        return false;
    }

    if (QThread::currentThread() == qApp->thread()) {
        return menuInstance && menuInstance->isMenuEnabled(menuOption);
    }

    bool result { false };

    BLOCKING_INVOKE_METHOD(menuInstance, "isMenuEnabled",
        Q_RETURN_ARG(bool, result),
        Q_ARG(const QString&, menuOption));

    return result;
}

void MenuScriptingInterface::setMenuEnabled(const QString& menuOption, bool isChecked) {
    Menu* menuInstance = Menu::getInstance();
    if (!menuInstance) {
        return;
    }

    QMetaObject::invokeMethod(menuInstance, "setMenuEnabled",
        Q_ARG(const QString&, menuOption),
        Q_ARG(bool, isChecked));
}

void MenuScriptingInterface::triggerOption(const QString& menuOption) {
    Menu* menuInstance = Menu::getInstance();
    if (!menuInstance) {
        return;
    }

    QMetaObject::invokeMethod(menuInstance, "triggerOption", Q_ARG(const QString&, menuOption));
}
