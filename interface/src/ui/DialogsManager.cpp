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
#include <ui/TabletScriptingInterface.h>

#include "AddressBarDialog.h"
#include "ConnectionFailureDialog.h"
#include "DomainConnectionDialog.h"
#include "HMDToolsDialog.h"
#include "LodToolsDialog.h"
#include "LoginDialog.h"
#include "OctreeStatsDialog.h"
#include "PreferencesDialog.h"
#include "UpdateDialog.h"

#include "scripting/HMDScriptingInterface.h"

static const QVariant TABLET_ADDRESS_DIALOG = "hifi/tablet/TabletAddressDialog.qml";
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

void DialogsManager::showAddressBar() {
    auto hmd = DependencyManager::get<HMDScriptingInterface>();
    auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
    auto tablet = dynamic_cast<TabletProxy*>(tabletScriptingInterface->getTablet("com.highfidelity.interface.tablet.system"));

    if (!tablet->isPathLoaded(TABLET_ADDRESS_DIALOG)) {
        tablet->loadQMLSource(TABLET_ADDRESS_DIALOG);
    }
    if (!hmd->getShouldShowTablet()) {
        hmd->openTablet();
    }
    qApp->setKeyboardFocusOverlay(hmd->getCurrentTabletScreenID());
    setAddressBarVisible(true);
}

void DialogsManager::hideAddressBar() {
    auto hmd = DependencyManager::get<HMDScriptingInterface>();
    auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
    auto tablet = dynamic_cast<TabletProxy*>(tabletScriptingInterface->getTablet("com.highfidelity.interface.tablet.system"));

    if (tablet->isPathLoaded(TABLET_ADDRESS_DIALOG)) {
        tablet->gotoHomeScreen();
        hmd->closeTablet();
    }
    qApp->setKeyboardFocusOverlay(UNKNOWN_OVERLAY_ID);
    setAddressBarVisible(false);
}

void DialogsManager::showFeed() {
    AddressBarDialog::show();
    emit setUseFeed(true);
}

void DialogsManager::setDomainConnectionFailureVisibility(bool visible) {
    qDebug() << "DialogsManager::setDomainConnectionFailureVisibility: visible" << visible;
    auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
    auto tablet = dynamic_cast<TabletProxy*>(tabletScriptingInterface->getTablet("com.highfidelity.interface.tablet.system"));

    if (tablet->getToolbarMode()) {
        if (visible) {
            ConnectionFailureDialog::show();
        } else {
            ConnectionFailureDialog::hide();
        }
    } else {
        static const QUrl url("dialogs/TabletConnectionFailureDialog.qml");
        auto hmd = DependencyManager::get<HMDScriptingInterface>();
        if (visible) {
            tablet->initialScreen(url);
            if (!hmd->getShouldShowTablet()) {
                hmd->openTablet();
            }
        }
    }
}

void DialogsManager::toggleLoginDialog() {
    LoginDialog::toggleAction();
}

void DialogsManager::showLoginDialog() {
    LoginDialog::showWithSelection();
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

void DialogsManager::toggleAddressBar() {
    auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
    auto tablet = dynamic_cast<TabletProxy*>(tabletScriptingInterface->getTablet("com.highfidelity.interface.tablet.system"));

    const bool addressBarLoaded = tablet->isPathLoaded(TABLET_ADDRESS_DIALOG);

    if (_addressBarVisible || addressBarLoaded) {
        hideAddressBar();
    } else {
        showAddressBar();
    }
}

void DialogsManager::setAddressBarVisible(bool addressBarVisible) {
    _addressBarVisible = addressBarVisible;
    emit addressBarShown(_addressBarVisible);
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
