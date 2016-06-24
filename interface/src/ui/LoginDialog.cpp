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

#include "AccountManager.h"
#include "DependencyManager.h"
#include "Menu.h"

HIFI_QML_DEF(LoginDialog)

LoginDialog::LoginDialog(QQuickItem *parent) : OffscreenQmlDialog(parent),
    _rootUrl(NetworkingConstants::METAVERSE_SERVER_URL.toString())
{
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

void LoginDialog::handleLoginCompleted(const QUrl&) {
    hide();
}

void LoginDialog::handleLoginFailed() {
    setStatusText("Invalid username or password");
}

void LoginDialog::setStatusText(const QString& statusText) {
    if (statusText != _statusText) {
        _statusText = statusText;
        emit statusTextChanged();
    }
}

QString LoginDialog::statusText() const {
    return _statusText;
}

QString LoginDialog::rootUrl() const {
    return _rootUrl;
}

void LoginDialog::login(const QString& username, const QString& password) {
    qDebug() << "Attempting to login " << username;
    setStatusText("Logging in...");
    DependencyManager::get<AccountManager>()->requestAccessToken(username, password);
}

void LoginDialog::openUrl(const QString& url) {
    qDebug() << url;
    QDesktopServices::openUrl(url);
}
