//
//  DialogsManager.cpp
//  interface/src/ui
//
//  Created by Clement on 1/18/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DialogsManager.h"

#include <QMessageBox>

#include <AccountManager.h>
#include <Application.h>
#include <MainWindow.h>
#include <PathUtils.h>

#include "AddressBarDialog.h"
#include "BandwidthDialog.h"
#include "CachesSizeDialog.h"
#include "ConnectionFailureDialog.h"
#include "DiskCacheEditor.h"
#include "DomainConnectionDialog.h"
#include "HMDToolsDialog.h"
#include "LodToolsDialog.h"
#include "LoginDialog.h"
#include "OctreeStatsDialog.h"
#include "PreferencesDialog.h"
#include "ScriptEditorWindow.h"
#include "SignupDialog.h"
#include "UpdateDialog.h"

template<typename T>
void DialogsManager::maybeCreateDialog(QPointer<T>& member) {
    if (!member) {
        MainWindow* parent = qApp->getWindow();
        Q_CHECK_PTR(parent);
        member = new T(parent);
        Q_CHECK_PTR(member);
        
        if (_hmdToolsDialog && member->windowHandle()) {
            _hmdToolsDialog->watchWindow(member->windowHandle());
        }
    }
}

void DialogsManager::toggleAddressBar() {
    AddressBarDialog::toggle();
    emit addressBarToggled();
}

void DialogsManager::showAddressBar() {
    AddressBarDialog::show();
}

void DialogsManager::showFeed() {
    AddressBarDialog::show();
    emit setUseFeed(true);
}

void DialogsManager::setDomainConnectionFailureVisibility(bool visible) {
    if (visible) {
        ConnectionFailureDialog::show();
    } else {
        ConnectionFailureDialog::hide();
    }
}

void DialogsManager::toggleDiskCacheEditor() {
    maybeCreateDialog(_diskCacheEditor);
    _diskCacheEditor->toggle();
}

void DialogsManager::toggleLoginDialog() {
    SignupDialog::show();
}

void DialogsManager::showLoginDialog() {
    LoginDialog::show();
}

void DialogsManager::showUpdateDialog() {
    UpdateDialog::show();
}

void DialogsManager::octreeStatsDetails() {
    if (!_octreeStatsDialog) {
        _octreeStatsDialog = new OctreeStatsDialog(qApp->getWindow(), qApp->getOcteeSceneStats());

        if (_hmdToolsDialog) {
            _hmdToolsDialog->watchWindow(_octreeStatsDialog->windowHandle());
        }
        connect(_octreeStatsDialog, SIGNAL(closed()), _octreeStatsDialog, SLOT(deleteLater()));
        _octreeStatsDialog->show();
    }
    _octreeStatsDialog->raise();
}

void DialogsManager::cachesSizeDialog() {
    if (!_cachesSizeDialog) {
        maybeCreateDialog(_cachesSizeDialog);

        connect(_cachesSizeDialog, SIGNAL(closed()), _cachesSizeDialog, SLOT(deleteLater()));
        _cachesSizeDialog->show();
    }
    _cachesSizeDialog->raise();
}

void DialogsManager::bandwidthDetails() {
    if (! _bandwidthDialog) {
        _bandwidthDialog = new BandwidthDialog(qApp->getWindow());
        connect(_bandwidthDialog, SIGNAL(closed()), _bandwidthDialog, SLOT(deleteLater()));

        if (_hmdToolsDialog) {
            _hmdToolsDialog->watchWindow(_bandwidthDialog->windowHandle());
        }

        _bandwidthDialog->show();
    }
    _bandwidthDialog->raise();
}

void DialogsManager::lodTools() {
    if (!_lodToolsDialog) {
        maybeCreateDialog(_lodToolsDialog);

        connect(_lodToolsDialog, SIGNAL(closed()), _lodToolsDialog, SLOT(deleteLater()));
        _lodToolsDialog->show();
    }
    _lodToolsDialog->raise();
}

void DialogsManager::hmdTools(bool showTools) {
    if (showTools) {
        if (!_hmdToolsDialog) {
            maybeCreateDialog(_hmdToolsDialog);
            connect(_hmdToolsDialog, SIGNAL(closed()), SLOT(hmdToolsClosed()));
        }
        _hmdToolsDialog->show();
        _hmdToolsDialog->raise();
    } else {
        hmdToolsClosed();
    }
    qApp->getWindow()->activateWindow();
}

void DialogsManager::hmdToolsClosed() {
    if (_hmdToolsDialog) {
        _hmdToolsDialog->hide();
    }
}

void DialogsManager::showScriptEditor() {
    maybeCreateDialog(_scriptEditor);
    _scriptEditor->show();
    _scriptEditor->raise();
}

void DialogsManager::showTestingResults() {
    if (!_testingDialog) {
        _testingDialog = new TestingDialog(qApp->getWindow());
        connect(_testingDialog, SIGNAL(closed()), _testingDialog, SLOT(deleteLater()));
    }
    _testingDialog->show();
    _testingDialog->raise();
}

void DialogsManager::showDomainConnectionDialog() {
    // if the dialog already exists we delete it so the connection data is refreshed
    if (_domainConnectionDialog) {
        _domainConnectionDialog->close();
        _domainConnectionDialog->deleteLater();
        _domainConnectionDialog = NULL;
    }

    maybeCreateDialog(_domainConnectionDialog);

    _domainConnectionDialog->show();
    _domainConnectionDialog->raise();
}
