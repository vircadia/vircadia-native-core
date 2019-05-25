//
//  KeyboardScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Dante Ruiz on 2018-08-27.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "KeyboardScriptingInterface.h"
#include "ui/Keyboard.h"

bool KeyboardScriptingInterface::isRaised() const {
    return DependencyManager::get<Keyboard>()->isRaised();
}

void KeyboardScriptingInterface::setRaised(bool raised) {
    DependencyManager::get<Keyboard>()->setRaised(raised);
}

bool KeyboardScriptingInterface::isPassword() const {
    return DependencyManager::get<Keyboard>()->isPassword();
}

void KeyboardScriptingInterface::setPassword(bool password) {
    DependencyManager::get<Keyboard>()->setPassword(password);
}

void KeyboardScriptingInterface::loadKeyboardFile(const QString& keyboardFile) {
    DependencyManager::get<Keyboard>()->loadKeyboardFile(keyboardFile);
}

bool KeyboardScriptingInterface::getUse3DKeyboard() const {
    return DependencyManager::get<Keyboard>()->getUse3DKeyboard();
}

void KeyboardScriptingInterface::disableRightMallet() {
    DependencyManager::get<Keyboard>()->disableRightMallet();
}

void KeyboardScriptingInterface::disableLeftMallet() {
    DependencyManager::get<Keyboard>()->disableLeftMallet();
}

void KeyboardScriptingInterface::enableRightMallet() {
    DependencyManager::get<Keyboard>()->enableRightMallet();
}

void KeyboardScriptingInterface::enableLeftMallet() {
    DependencyManager::get<Keyboard>()->enableLeftMallet();
}

void KeyboardScriptingInterface::setLeftHandLaser(unsigned int leftHandLaser) {
    DependencyManager::get<Keyboard>()->setLeftHandLaser(leftHandLaser);
}

void KeyboardScriptingInterface::setRightHandLaser(unsigned int rightHandLaser) {
    DependencyManager::get<Keyboard>()->setRightHandLaser(rightHandLaser);
}

bool KeyboardScriptingInterface::getPreferMalletsOverLasers() const {
    return DependencyManager::get<Keyboard>()->getPreferMalletsOverLasers();
}

bool KeyboardScriptingInterface::containsID(const QUuid& id) const {
    return DependencyManager::get<Keyboard>()->containsID(id);
}
