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
#include <QJsonDocument>
#include <QNetworkReply>

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

bool LoginDialog::isSteamRunning() const {
    return SteamClient::isRunning();
}

void LoginDialog::login(const QString& username, const QString& password) const {
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

void LoginDialog::linkSteam() {
    qDebug() << "Attempting to link Steam account";
    SteamClient::requestTicket([this](Ticket ticket) {
        if (ticket.isNull()) {
            emit handleLoginFailed();
            return;
        }

        JSONCallbackParameters callbackParams;
        callbackParams.jsonCallbackReceiver = this;
        callbackParams.jsonCallbackMethod = "linkCompleted";
        callbackParams.errorCallbackReceiver = this;
        callbackParams.errorCallbackMethod = "linkFailed";

        const QString LINK_STEAM_PATH = "api/v1/user/link_steam";

        QJsonObject payload;
        payload.insert("steam_auth_ticket", QJsonValue::fromVariant(QVariant(ticket)));

        auto accountManager = DependencyManager::get<AccountManager>();
        accountManager->sendRequest(LINK_STEAM_PATH, AccountManagerAuth::Required,
                                    QNetworkAccessManager::PostOperation, callbackParams,
                                    QJsonDocument(payload).toJson());
    });
}

void LoginDialog::createAccountFromStream(QString username) {
    qDebug() << "Attempting to create account from Steam info";
    SteamClient::requestTicket([this, username](Ticket ticket) {
        if (ticket.isNull()) {
            emit handleLoginFailed();
            return;
        }

        JSONCallbackParameters callbackParams;
        callbackParams.jsonCallbackReceiver = this;
        callbackParams.jsonCallbackMethod = "createCompleted";
        callbackParams.errorCallbackReceiver = this;
        callbackParams.errorCallbackMethod = "createFailed";

        const QString CREATE_ACCOUNT_FROM_STEAM_PATH = "api/v1/user/create_from_steam";

        QJsonObject payload;
        payload.insert("steam_auth_ticket", QJsonValue::fromVariant(QVariant(ticket)));
        if (!username.isEmpty()) {
            payload.insert("username", QJsonValue::fromVariant(QVariant(username)));
        }

        auto accountManager = DependencyManager::get<AccountManager>();
        accountManager->sendRequest(CREATE_ACCOUNT_FROM_STEAM_PATH, AccountManagerAuth::None,
                                    QNetworkAccessManager::PostOperation, callbackParams,
                                    QJsonDocument(payload).toJson());
    });

}

void LoginDialog::openUrl(const QString& url) const {
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    auto browser = offscreenUi->load("Browser.qml");
    browser->setProperty("url", url);
}

void LoginDialog::linkCompleted(QNetworkReply& reply) {
    emit handleLinkCompleted();
}

void LoginDialog::linkFailed(QNetworkReply& reply) {
    emit handleLinkFailed(reply.errorString());
}

void LoginDialog::createCompleted(QNetworkReply& reply) {
    emit handleCreateCompleted();
}

void LoginDialog::createFailed(QNetworkReply& reply) {
    emit handleCreateFailed(reply.errorString());
}

