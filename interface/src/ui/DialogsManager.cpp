//
//  DialogsManager.cpp
//
//
//  Created by Clement on 1/18/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <Application.h>
#include <MainWindow.h>

#include "AddressBarDialog.h"
#include "LoginDialog.h"
#include "OctreeStatsDialog.h"

#include "DialogsManager.h"

void DialogsManager::toggleAddressBar() {
    maybeCreateDialog(_addressBarDialog);
    
    if (!_addressBarDialog->isVisible()) {
        _addressBarDialog->show();
    }
}

void DialogsManager::toggleLoginDialog() {
    maybeCreateDialog(_loginDialog);
    _loginDialog->toggleQAction();
}

void DialogsManager::showLoginDialog() {
    maybeCreateDialog(_loginDialog);
    _loginDialog->showLoginForCurrentDomain();
}

void DialogsManager::octreeStatsDetails() {
    if (!_octreeStatsDialog) {
        _octreeStatsDialog = new OctreeStatsDialog(DependencyManager::get<GLCanvas>().data(),
                                                   Application::getInstance()->getOcteeSceneStats());
        connect(_octreeStatsDialog, SIGNAL(closed()), _octreeStatsDialog, SLOT(deleteLater()));
        _octreeStatsDialog->show();
        //TODO: wire hmdToolsDialog once moved
//        if (_hmdToolsDialog) {
//            _hmdToolsDialog->watchWindow(_octreeStatsDialog->windowHandle());
//        }
    }
    _octreeStatsDialog->raise();
}

