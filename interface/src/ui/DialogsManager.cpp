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
#include <MainWindow.h>
#include <PathUtils.h>

#include "AddressBarDialog.h"
#include "AnimationsDialog.h"
#include "AttachmentsDialog.h"
#include "BandwidthDialog.h"
#include "CachesSizeDialog.h"
#include "DiskCacheEditor.h"
#include "DomainConnectionDialog.h"
#include "HMDToolsDialog.h"
#include "LodToolsDialog.h"
#include "LoginDialog.h"
#include "OctreeStatsDialog.h"
#include "PreferencesDialog.h"
#include "ScriptEditorWindow.h"
#include "UpdateDialog.h"


void DialogsManager::toggleAddressBar() {
    AddressBarDialog::toggle();
    emit addressBarToggled();
}

void DialogsManager::toggleDiskCacheEditor() {
    maybeCreateDialog(_diskCacheEditor);
    _diskCacheEditor->toggle();
}

void DialogsManager::toggleLoginDialog() {
    LoginDialog::toggleAction();
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

void DialogsManager::audioStatsDetails() {
    if (! _audioStatsDialog) {
        _audioStatsDialog = new AudioStatsDialog(qApp->getWindow());
        connect(_audioStatsDialog, SIGNAL(closed()), _audioStatsDialog, SLOT(deleteLater()));
        
        if (_hmdToolsDialog) {
            _hmdToolsDialog->watchWindow(_audioStatsDialog->windowHandle());
        }
        
        _audioStatsDialog->show();
    }
    _audioStatsDialog->raise();
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
    _hmdToolsDialog->hide();
}

void DialogsManager::showScriptEditor() {
    maybeCreateDialog(_scriptEditor);
    _scriptEditor->show();
    _scriptEditor->raise();
}

void DialogsManager::showIRCLink() {
    if (!_ircInfoBox) {
        _ircInfoBox = new QMessageBox(QMessageBox::NoIcon,
                                      "High Fidelity IRC",
                                      "High Fidelity has an IRC channel on irc.freenode.net at #highfidelity.<br/><br/>Web chat is available <a href='http://webchat.freenode.net/?channels=highfidelity&uio=d4'>here</a>.",
                                      QMessageBox::Ok);
        _ircInfoBox->setTextFormat(Qt::RichText);
        _ircInfoBox->setAttribute(Qt::WA_DeleteOnClose);
        _ircInfoBox->show();
    }

    _ircInfoBox->raise();
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
