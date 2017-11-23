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
    QMetaObject::invokeMethod(Menu::getInstance(), "addMenu", Q_ARG(const QString&, menu), Q_ARG(const QString&, grouping));
}

void MenuScriptingInterface::removeMenu(const QString& menu) {
    QMetaObject::invokeMethod(Menu::getInstance(), "removeMenu", Q_ARG(const QString&, menu));
}

bool MenuScriptingInterface::menuExists(const QString& menu) {
    if (QThread::currentThread() == qApp->thread()) {
        return Menu::getInstance()->menuExists(menu);
    }
    bool result;
    BLOCKING_INVOKE_METHOD(Menu::getInstance(), "menuExists",
                Q_RETURN_ARG(bool, result), 
                Q_ARG(const QString&, menu));
    return result;
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

bool MenuScriptingInterface::menuItemExists(const QString& menu, const QString& menuitem) {
    if (QThread::currentThread() == qApp->thread()) {
        return Menu::getInstance()->menuItemExists(menu, menuitem);
    }
    bool result;
    BLOCKING_INVOKE_METHOD(Menu::getInstance(), "menuItemExists",
        Q_RETURN_ARG(bool, result),
        Q_ARG(const QString&, menu),
        Q_ARG(const QString&, menuitem));
    return result;
}

void MenuScriptingInterface::addActionGroup(const QString& groupName, const QStringList& actionList,
                                            const QString& selected) {
    static const char* slot = SLOT(menuItemTriggered());
    QMetaObject::invokeMethod(Menu::getInstance(), "addActionGroup",
                              Q_ARG(const QString&, groupName),
                              Q_ARG(const QStringList&, actionList),
                              Q_ARG(const QString&, selected),
                              Q_ARG(QObject*, this),
                              Q_ARG(const char*, slot));
}

void MenuScriptingInterface::removeActionGroup(const QString& groupName) {
    QMetaObject::invokeMethod(Menu::getInstance(), "removeActionGroup",
                              Q_ARG(const QString&, groupName));
}

bool MenuScriptingInterface::isOptionChecked(const QString& menuOption) {
    if (QThread::currentThread() == qApp->thread()) {
        return Menu::getInstance()->isOptionChecked(menuOption);
    }
    bool result;
    BLOCKING_INVOKE_METHOD(Menu::getInstance(), "isOptionChecked",
                Q_RETURN_ARG(bool, result), 
                Q_ARG(const QString&, menuOption));
    return result;
}

void MenuScriptingInterface::setIsOptionChecked(const QString& menuOption, bool isChecked) {
    QMetaObject::invokeMethod(Menu::getInstance(), "setIsOptionChecked",
                Q_ARG(const QString&, menuOption),
                Q_ARG(bool, isChecked));
}

bool MenuScriptingInterface::isMenuEnabled(const QString& menuOption) {
    if (QThread::currentThread() == qApp->thread()) {
        return Menu::getInstance()->isOptionChecked(menuOption);
    }
    bool result;
    BLOCKING_INVOKE_METHOD(Menu::getInstance(), "isMenuEnabled",
        Q_RETURN_ARG(bool, result),
        Q_ARG(const QString&, menuOption));
    return result;
}

void MenuScriptingInterface::setMenuEnabled(const QString& menuOption, bool isChecked) {
    QMetaObject::invokeMethod(Menu::getInstance(), "setMenuEnabled",
        Q_ARG(const QString&, menuOption),
        Q_ARG(bool, isChecked));
}

void MenuScriptingInterface::triggerOption(const QString& menuOption) {
    QMetaObject::invokeMethod(Menu::getInstance(), "triggerOption", Q_ARG(const QString&, menuOption));    
}

void MenuScriptingInterface::closeInfoView(const QString& path) {
    QMetaObject::invokeMethod(Menu::getInstance(), "closeInfoView", Q_ARG(const QString&, path));
}

bool MenuScriptingInterface::isInfoViewVisible(const QString& path) {
    if (QThread::currentThread() == qApp->thread()) {
        return Menu::getInstance()->isInfoViewVisible(path);
    }
        
    bool result;
    BLOCKING_INVOKE_METHOD(Menu::getInstance(), "isInfoViewVisible",
        Q_RETURN_ARG(bool, result), Q_ARG(const QString&, path));
    return result;
}

