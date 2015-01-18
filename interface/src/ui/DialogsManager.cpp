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
#include "AnimationsDialog.h"
#include "AttachmentsDialog.h"
#include "CachesSizeDialog.h"
#include "LoginDialog.h"
#include "OctreeStatsDialog.h"
#include "PreferencesDialog.h"

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

void DialogsManager::cachesSizeDialog() {
    qDebug() << "Caches size:" << _cachesSizeDialog.isNull();
    if (!_cachesSizeDialog) {
        maybeCreateDialog(_cachesSizeDialog);
        
        connect(_cachesSizeDialog, SIGNAL(closed()), _cachesSizeDialog, SLOT(deleteLater()));
        _cachesSizeDialog->show();
        
        //TODO: wire hmdToolsDialog once moved
//        if (_hmdToolsDialog) {
//            _hmdToolsDialog->watchWindow(_cachesSizeDialog->windowHandle());
//        }
    }
    _cachesSizeDialog->raise();
}

void DialogsManager::editPreferences() {
    if (!_preferencesDialog) {
        maybeCreateDialog(_preferencesDialog);
        _preferencesDialog->show();
    } else {
        _preferencesDialog->close();
    }
}

void DialogsManager::editAttachments() {
    if (!_attachmentsDialog) {
        maybeCreateDialog(_attachmentsDialog);
        _attachmentsDialog->show();
    } else {
        _attachmentsDialog->close();
    }
}

void DialogsManager::editAnimations() {
    if (!_animationsDialog) {
        maybeCreateDialog(_animationsDialog);
        _animationsDialog->show();
    } else {
        _animationsDialog->close();
    }
}

