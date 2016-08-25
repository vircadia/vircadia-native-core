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
    connect(DependencyManager::get<DialogsManager>().data(), &DialogsManager::addressBarToggled,
            this, &DialogsManagerScriptingInterface::addressBarToggled);
    connect(DependencyManager::get<DialogsManager>().data(), &DialogsManager::addressBarShown,
            this, &DialogsManagerScriptingInterface::addressBarShown);
}

void DialogsManagerScriptingInterface::toggleAddressBar() {
    QMetaObject::invokeMethod(DependencyManager::get<DialogsManager>().data(),
                              "toggleAddressBar", Qt::QueuedConnection);
}

void DialogsManagerScriptingInterface::showFeed() {
    QMetaObject::invokeMethod(DependencyManager::get<DialogsManager>().data(),
        "showFeed", Qt::QueuedConnection);
}
