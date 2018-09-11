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

#include <QtGui/QDesktopServices>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtNetwork/QNetworkReply>

#include <plugins/PluginManager.h>
#include <plugins/SteamClientPlugin.h>
#include <ui/TabletScriptingInterface.h>
#include <UserActivityLogger.h>

#include "AccountManager.h"
#include "DependencyManager.h"
#include "Menu.h"

#include "Application.h"
#include "scripting/HMDScriptingInterface.h"

HIFI_QML_DEF(LoginDialog)

LoginDialog::LoginDialog(QQuickItem *parent) : OffscreenQmlDialog(parent) {
    auto accountManager = DependencyManager::get<AccountManager>();
#if !defined(Q_OS_ANDROID)
    connect(accountManager.data(), &AccountManager::loginComplete,
        this, &LoginDialog::handleLoginCompleted);
    connect(accountManager.data(), &AccountManager::loginFailed,
            this, &LoginDialog::handleLoginFailed);
#endif
}

LoginDialog::~LoginDialog() {
    Setting::Handle<bool> loginDialogPoppedUp{ "loginDialogPoppedUp", false };
    if (loginDialogPoppedUp.get()) {
        QJsonObject data;
        data["action"] = "user opted out";
        UserActivityLogger::getInstance().logAction("encourageLoginDialog", data);
    }
    loginDialogPoppedUp.set(false);
}

void LoginDialog::showWithSelection() {
    auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
    auto tablet = dynamic_cast<TabletProxy*>(tabletScriptingInterface->getTablet("com.highfidelity.interface.tablet.system"));
    auto hmd = DependencyManager::get<HMDScriptingInterface>();

    if (tablet->getToolbarMode()) {
        LoginDialog::show();
    } else {
        static const QUrl url("dialogs/TabletLoginDialog.qml");
        tablet->initialScreen(url);
        if (!hmd->getShouldShowTablet()) {
            hmd->openTablet();
        }
    }
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
        loginAction->setText("Login / Sign Up");
        connection = connect(loginAction, &QAction::triggered, [] { LoginDialog::showWithSelection(); });
    }
}

bool LoginDialog::isSteamRunning() const {
    auto steamClient = PluginManager::getInstance()->getSteamClientPlugin();
    return steamClient && steamClient->isRunning();
}

void LoginDialog::login(const QString& username, const QString& password) const {
    qDebug() << "Attempting to login " << username;
    DependencyManager::get<AccountManager>()->requestAccessToken(username, password);
}

void LoginDialog::loginThroughSteam() {
    qDebug() << "Attempting to login through Steam";
    if (auto steamClient = PluginManager::getInstance()->getSteamClientPlugin()) {
        steamClient->requestTicket([this](Ticket ticket) {
            if (ticket.isNull()) {
                emit handleLoginFailed();
                return;
            }

            DependencyManager::get<AccountManager>()->requestAccessTokenWithSteam(ticket);
        });
    }
}

void LoginDialog::linkSteam() {
    qDebug() << "Attempting to link Steam account";
    if (auto steamClient = PluginManager::getInstance()->getSteamClientPlugin()) {
        steamClient->requestTicket([this](Ticket ticket) {
            if (ticket.isNull()) {
                emit handleLoginFailed();
                return;
            }

            JSONCallbackParameters callbackParams;
            callbackParams.callbackReceiver = this;
            callbackParams.jsonCallbackMethod = "linkCompleted";
            callbackParams.errorCallbackMethod = "linkFailed";

            const QString LINK_STEAM_PATH = "api/v1/user/steam/link";

            QJsonObject payload;
            payload.insert("steam_auth_ticket", QJsonValue::fromVariant(QVariant(ticket)));

            auto accountManager = DependencyManager::get<AccountManager>();
            accountManager->sendRequest(LINK_STEAM_PATH, AccountManagerAuth::Required,
                                        QNetworkAccessManager::PostOperation, callbackParams,
                                        QJsonDocument(payload).toJson());
        });
    }
}

void LoginDialog::createAccountFromStream(QString username) {
    qDebug() << "Attempting to create account from Steam info";
    if (auto steamClient = PluginManager::getInstance()->getSteamClientPlugin()) {
        steamClient->requestTicket([this, username](Ticket ticket) {
            if (ticket.isNull()) {
                emit handleLoginFailed();
                return;
            }

            JSONCallbackParameters callbackParams;
            callbackParams.callbackReceiver = this;
            callbackParams.jsonCallbackMethod = "createCompleted";
            callbackParams.errorCallbackMethod = "createFailed";

            const QString CREATE_ACCOUNT_FROM_STEAM_PATH = "api/v1/user/steam/create";

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
}

void LoginDialog::openUrl(const QString& url) const {
    auto tablet = dynamic_cast<TabletProxy*>(DependencyManager::get<TabletScriptingInterface>()->getTablet("com.highfidelity.interface.tablet.system"));
    auto hmd = DependencyManager::get<HMDScriptingInterface>();
    auto offscreenUi = DependencyManager::get<OffscreenUi>();

    if (tablet->getToolbarMode()) {
        offscreenUi->load("Browser.qml", [=](QQmlContext* context, QObject* newObject) {
            newObject->setProperty("url", url);
        });
        LoginDialog::hide();
    } else {
        if (!hmd->getShouldShowTablet() && !qApp->isHMDMode()) {
            offscreenUi->load("Browser.qml", [=](QQmlContext* context, QObject* newObject) {
                newObject->setProperty("url", url);
            });
            LoginDialog::hide();
        } else {
            tablet->gotoWebScreen(url);
        }
    }
}

void LoginDialog::linkCompleted(QNetworkReply* reply) {
    emit handleLinkCompleted();
}

void LoginDialog::linkFailed(QNetworkReply* reply) {
    emit handleLinkFailed(reply->errorString());
}

void LoginDialog::createCompleted(QNetworkReply* reply) {
    emit handleCreateCompleted();
}

void LoginDialog::createFailed(QNetworkReply* reply) {
    emit handleCreateFailed(reply->errorString());
}

void LoginDialog::signup(const QString& email, const QString& username, const QString& password) {
    JSONCallbackParameters callbackParams;
    callbackParams.callbackReceiver = this;
    callbackParams.jsonCallbackMethod = "signupCompleted";
    callbackParams.errorCallbackMethod = "signupFailed";

    QJsonObject payload;

    QJsonObject userObject;
    userObject.insert("email", email);
    userObject.insert("username", username);
    userObject.insert("password", password);

    payload.insert("user", userObject);

    static const QString API_SIGNUP_PATH = "api/v1/users";

    qDebug() << "Sending a request to create an account for" << username;

    auto accountManager = DependencyManager::get<AccountManager>();
    accountManager->sendRequest(API_SIGNUP_PATH, AccountManagerAuth::None,
                                QNetworkAccessManager::PostOperation, callbackParams,
                                QJsonDocument(payload).toJson());
}

void LoginDialog::signupCompleted(QNetworkReply* reply) {
    emit handleSignupCompleted();
}

QString errorStringFromAPIObject(const QJsonValue& apiObject) {
    if (apiObject.isArray()) {
        return apiObject.toArray()[0].toString();
    } else if (apiObject.isString()) {
        return apiObject.toString();
    } else {
        return "is invalid";
    }
}

void LoginDialog::signupFailed(QNetworkReply* reply) {
    // parse the returned JSON to see what the problem was
    auto jsonResponse = QJsonDocument::fromJson(reply->readAll());

    static const QString RESPONSE_DATA_KEY = "data";

    auto dataJsonValue = jsonResponse.object()[RESPONSE_DATA_KEY];

    if (dataJsonValue.isObject()) {
        auto dataObject = dataJsonValue.toObject();

        static const QString EMAIL_DATA_KEY = "email";
        static const QString USERNAME_DATA_KEY = "username";
        static const QString PASSWORD_DATA_KEY = "password";

        QStringList errorStringList;

        if (dataObject.contains(EMAIL_DATA_KEY)) {
            errorStringList.append(QString("Email %1.").arg(errorStringFromAPIObject(dataObject[EMAIL_DATA_KEY])));
        }

        if (dataObject.contains(USERNAME_DATA_KEY)) {
            errorStringList.append(QString("Username %1.").arg(errorStringFromAPIObject(dataObject[USERNAME_DATA_KEY])));
        }

        if (dataObject.contains(PASSWORD_DATA_KEY)) {
            errorStringList.append(QString("Password %1.").arg(errorStringFromAPIObject(dataObject[PASSWORD_DATA_KEY])));
        }

        emit handleSignupFailed(errorStringList.join('\n'));
    } else {
        static const QString DEFAULT_SIGN_UP_FAILURE_MESSAGE = "There was an unknown error while creating your account. Please try again later.";
        emit handleSignupFailed(DEFAULT_SIGN_UP_FAILURE_MESSAGE);
    }
}
