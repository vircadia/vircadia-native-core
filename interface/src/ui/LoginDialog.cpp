//
//
//  LoginDialog.cpp
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "LoginDialog.h"

#include "DependencyManager.h"
#include "AccountManager.h"
#include "Menu.h"
#include <NetworkingConstants.h>

HIFI_QML_DEF(LoginDialog)

LoginDialog::LoginDialog(QQuickItem *parent) : OffscreenQmlDialog(parent), _rootUrl(NetworkingConstants::METAVERSE_SERVER_URL.toString()) {
    connect(&AccountManager::getInstance(), &AccountManager::loginComplete,
        this, &LoginDialog::handleLoginCompleted);
    connect(&AccountManager::getInstance(), &AccountManager::loginFailed,
        this, &LoginDialog::handleLoginFailed);
}

void LoginDialog::toggleAction() {
    AccountManager & accountManager = AccountManager::getInstance();
    Menu* menu = Menu::getInstance();
    if (accountManager.isLoggedIn()) {
        // change the menu item to logout
        menu->setOptionText(MenuOption::Login, "Logout " + accountManager.getAccountInfo().getUsername());
        menu->setOptionTriggerAction(MenuOption::Login, [] {
            AccountManager::getInstance().logout();
        });
    } else {
        // change the menu item to login
        menu->setOptionText(MenuOption::Login, "Login");
        menu->setOptionTriggerAction(MenuOption::Login, [] {
            LoginDialog::show();
        });
    }
}

void LoginDialog::handleLoginCompleted(const QUrl&) {
    hide();
}

void LoginDialog::handleLoginFailed() {
    setStatusText("<font color = \"#267077\">Invalid username or password.< / font>");
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
    setStatusText("Authenticating...");
    AccountManager::getInstance().requestAccessToken(username, password);
}

void LoginDialog::openUrl(const QString& url) {
    qDebug() << url;
}
