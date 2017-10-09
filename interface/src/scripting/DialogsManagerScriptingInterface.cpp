//
//  DialogsManagerScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Zander Otavka on 7/17/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DialogsManagerScriptingInterface.h"

#include <DependencyManager.h>

#include "ui/DialogsManager.h"

DialogsManagerScriptingInterface::DialogsManagerScriptingInterface() {
    connect(DependencyManager::get<DialogsManager>().data(), &DialogsManager::addressBarShown,
            this, &DialogsManagerScriptingInterface::addressBarShown);
}


DialogsManagerScriptingInterface* DialogsManagerScriptingInterface::getInstance() {
    static DialogsManagerScriptingInterface sharedInstance;
    return &sharedInstance;
}

void DialogsManagerScriptingInterface::showAddressBar() {
    QMetaObject::invokeMethod(DependencyManager::get<DialogsManager>().data(),
        "showAddressBar", Qt::QueuedConnection);
}

void DialogsManagerScriptingInterface::hideAddressBar() {
    QMetaObject::invokeMethod(DependencyManager::get<DialogsManager>().data(),
                              "hideAddressBar", Qt::QueuedConnection);
}

void DialogsManagerScriptingInterface::showLoginDialog() {
    QMetaObject::invokeMethod(DependencyManager::get<DialogsManager>().data(),
        "showLoginDialog", Qt::QueuedConnection);
}

void DialogsManagerScriptingInterface::showFeed() {
    QMetaObject::invokeMethod(DependencyManager::get<DialogsManager>().data(),
        "showFeed", Qt::QueuedConnection);
}
