//
//  LoginDialog.cpp
//  interface/src/ui
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "LoginDialog.h"

#include <QDesktopServices>

#include <NetworkingConstants.h>
#include <steamworks-wrapper/SteamClient.h>

#include "AccountManager.h"
#include "DependencyManager.h"
#include "Menu.h"

HIFI_QML_DEF(LoginDialog)

LoginDialog::LoginDialog(QQuickItem *parent) : OffscreenQmlDialog(parent) {
    auto accountManager = DependencyManager::get<AccountManager>();
    connect(accountManager.data(), &AccountManager::loginComplete,
        this, &LoginDialog::handleLoginCompleted);
    connect(accountManager.data(), &AccountManager::loginFailed,
        this, &LoginDialog::handleLoginFailed);
}

void LoginDialog::toggleAction() {
    auto accountManager = DependencyManager::get<AccountManager>();
    QAction* loginAction = Menu::getInstance()->getActionForOption(MenuOption::Login);
    Q_CHECK_PTR(loginAction);
    static QMetaObject::Connection connection;
    if (connection) {
        disconnect(connection);
    }

    if (accountManager->isLoggedIn()) {
        // change the menu item to logout
        loginAction->setText("Logout " + accountManager->getAccountInfo().getUsername());
        connection = connect(loginAction, &QAction::triggered, accountManager.data(), &AccountManager::logout);
    } else {
        // change the menu item to login
        loginAction->setText("Login");
        connection = connect(loginAction, &QAction::triggered, [] {
            LoginDialog::show();
        });
    }
}

void LoginDialog::login(const QString& username, const QString& password) {
    qDebug() << "Attempting to login " << username;
    DependencyManager::get<AccountManager>()->requestAccessToken(username, password);
}

void LoginDialog::loginThroughSteam() {
    qDebug() << "Attempting to login through Steam";
    SteamClient::requestTicket([this](Ticket ticket) {
        if (ticket.isNull()) {
            emit handleLoginFailed();
            return;
        }

        DependencyManager::get<AccountManager>()->requestAccessTokenWithSteam(ticket);
    });
}

void LoginDialog::openUrl(const QString& url) {
    QDesktopServices::openUrl(url);
}
