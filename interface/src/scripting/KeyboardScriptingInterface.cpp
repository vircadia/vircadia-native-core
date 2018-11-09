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

bool KeyboardScriptingInterface::isRaised() {
    return DependencyManager::get<Keyboard>()->isRaised();
}

void KeyboardScriptingInterface::setRaised(bool raised) {
    DependencyManager::get<Keyboard>()->setRaised(raised);
}


bool KeyboardScriptingInterface::isPassword() {
    return DependencyManager::get<Keyboard>()->isPassword();
}

void KeyboardScriptingInterface::setPassword(bool password) {
    DependencyManager::get<Keyboard>()->setPassword(password);
}

void KeyboardScriptingInterface::loadKeyboardFile(const QString& keyboardFile) {
    DependencyManager::get<Keyboard>()->loadKeyboardFile(keyboardFile);
}
