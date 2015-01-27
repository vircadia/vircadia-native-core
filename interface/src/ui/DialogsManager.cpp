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

#include <AccountManager.h>
#include <MainWindow.h>
#include <PathUtils.h>
#include <XmppClient.h>

#include "AddressBarDialog.h"
#include "AnimationsDialog.h"
#include "AttachmentsDialog.h"
#include "BandwidthDialog.h"
#include "CachesSizeDialog.h"
#include "ChatWindow.h"
#include "HMDToolsDialog.h"
#include "LodToolsDialog.h"
#include "LoginDialog.h"
#include "MetavoxelEditor.h"
#include "MetavoxelNetworkSimulator.h"
#include "OctreeStatsDialog.h"
#include "PreferencesDialog.h"
#include "ScriptEditorWindow.h"

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
    qDebug() << "Caches size:" << _cachesSizeDialog.isNull();
    if (!_cachesSizeDialog) {
        maybeCreateDialog(_cachesSizeDialog);
        
        connect(_cachesSizeDialog, SIGNAL(closed()), _cachesSizeDialog, SLOT(deleteLater()));
        _cachesSizeDialog->show();
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

void DialogsManager::bandwidthDetails() {
    if (! _bandwidthDialog) {
        _bandwidthDialog = new BandwidthDialog(qApp->getWindow(), qApp->getBandwidthMeter());
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

void DialogsManager::toggleToolWindow() {
    QMainWindow* toolWindow = qApp->getToolWindow();
    toolWindow->setVisible(!toolWindow->isVisible());
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
    Menu::getInstance()->getActionForOption(MenuOption::HMDTools)->setChecked(false);
    _hmdToolsDialog->hide();
}

void DialogsManager::showMetavoxelEditor() {
    maybeCreateDialog(_metavoxelEditor);
    _metavoxelEditor->raise();
}

void DialogsManager::showMetavoxelNetworkSimulator() {
    maybeCreateDialog(_metavoxelNetworkSimulator);
    _metavoxelNetworkSimulator->raise();
}

void DialogsManager::showScriptEditor() {
    maybeCreateDialog(_scriptEditor);
    _scriptEditor->raise();
}

void DialogsManager::setupChat() {
#ifdef HAVE_QXMPP
    const QXmppClient& xmppClient = XmppClient::getInstance().getXMPPClient();
    connect(&xmppClient, &QXmppClient::connected, this, &DialogsManager::toggleChat);
    connect(&xmppClient, &QXmppClient::disconnected, this, &DialogsManager::toggleChat);
    
    QDir::setCurrent(PathUtils::resourcesPath());
    // init chat window to listen chat
    maybeCreateDialog(_chatWindow);
#endif
}

void DialogsManager::showChat() {
    if (AccountManager::getInstance().isLoggedIn()) {
        maybeCreateDialog(_chatWindow);
        
        if (_chatWindow->isHidden()) {
            _chatWindow->show();
        }
        _chatWindow->raise();
        _chatWindow->activateWindow();
        _chatWindow->setFocus();
    } else {
        qApp->getTrayIcon()->showMessage("Interface",
                                         "You need to login to be able to chat with others on this domain.");
    }
}

void DialogsManager::toggleChat() {
#ifdef HAVE_QXMPP
    QAction* chatAction = Menu::getInstance()->getActionForOption(MenuOption::Login);
    Q_CHECK_PTR(chatAction);
    
    chatAction->setEnabled(XmppClient::getInstance().getXMPPClient().isConnected());
    if (!chatAction->isEnabled() && _chatWindow && AccountManager::getInstance().isLoggedIn()) {
        if (_chatWindow->isHidden()) {
            _chatWindow->show();
            _chatWindow->raise();
            _chatWindow->activateWindow();
            _chatWindow->setFocus();
        } else {
            _chatWindow->hide();
        }
    }
#endif
}




