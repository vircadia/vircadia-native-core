//
//  LoginDialog.cpp
//  interface/src/ui
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
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
#include <plugins/OculusPlatformPlugin.h>
#include <shared/GlobalAppProperties.h>
#include <ui/TabletScriptingInterface.h>
#include <UserActivityLogger.h>

#include "AccountManager.h"
#include "DomainAccountManager.h"
#include "DependencyManager.h"
#include "DialogsManager.h"
#include "Menu.h"

#include "Application.h"
#include "scripting/HMDScriptingInterface.h"
#include "Constants.h"

HIFI_QML_DEF(LoginDialog)

static const QUrl TABLET_LOGIN_DIALOG_URL("dialogs/TabletLoginDialog.qml");
const QUrl LOGIN_DIALOG = PathUtils::qmlUrl("OverlayLoginDialog.qml");

LoginDialog::LoginDialog(QQuickItem *parent) : OffscreenQmlDialog(parent) {
    auto accountManager = DependencyManager::get<AccountManager>();
    auto domainAccountManager = DependencyManager::get<DomainAccountManager>();
    // the login hasn't been dismissed yet if the user isn't logged in and is encouraged to login.
#if !defined(Q_OS_ANDROID)
    connect(accountManager.data(), &AccountManager::loginComplete,
        this, &LoginDialog::handleLoginCompleted);
    connect(accountManager.data(), &AccountManager::loginFailed,
            this, &LoginDialog::handleLoginFailed);
    connect(domainAccountManager.data(), &DomainAccountManager::loginComplete,
        this, &LoginDialog::handleLoginCompleted);
    connect(domainAccountManager.data(), &DomainAccountManager::loginFailed,
            this, &LoginDialog::handleLoginFailed);
    connect(qApp, &Application::loginDialogFocusEnabled, this, &LoginDialog::focusEnabled);
    connect(qApp, &Application::loginDialogFocusDisabled, this, &LoginDialog::focusDisabled);
    connect(this, SIGNAL(dismissedLoginDialog()), qApp, SLOT(onDismissedLoginDialog()));
#endif
}

LoginDialog::~LoginDialog() {
}

void LoginDialog::showWithSelection() {
    auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
    auto tablet = dynamic_cast<TabletProxy*>(tabletScriptingInterface->getTablet("com.highfidelity.interface.tablet.system"));
    auto hmd = DependencyManager::get<HMDScriptingInterface>();

    if (!qApp->isHMDMode()) {
        if (qApp->getLoginDialogPoppedUp()) {
            LoginDialog::show();
            return;
        } else {
            if (!tablet->isPathLoaded(TABLET_LOGIN_DIALOG_URL)) {
                tablet->loadQMLSource(TABLET_LOGIN_DIALOG_URL);
            }
        }
    } else {
        if (!qApp->getLoginDialogPoppedUp()) {
            tablet->initialScreen(TABLET_LOGIN_DIALOG_URL);
        } else {
            qApp->createLoginDialog();
        }
    }

    if (!hmd->getShouldShowTablet()) {
        hmd->openTablet();
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
        loginAction->setText("Metaverse: Logout " + accountManager->getAccountInfo().getUsername());
        connection = connect(loginAction, &QAction::triggered, accountManager.data(), &AccountManager::logout);
    } else {
        // change the menu item to login
        loginAction->setText("Metaverse: Log In / Sign Up");
        connection = connect(loginAction, &QAction::triggered, [] {
            // if not in login state, show.
            if (!qApp->getLoginDialogPoppedUp()) {
                auto dialogsManager = DependencyManager::get<DialogsManager>();
                dialogsManager->setMetaverseLoginState();
                LoginDialog::showWithSelection();
            }
        });
    }
}

bool LoginDialog::isSteamRunning() const {
    auto steamClient = PluginManager::getInstance()->getSteamClientPlugin();
    return steamClient && steamClient->isRunning();
}

bool LoginDialog::isOculusRunning() const {
    auto oculusPlatformPlugin = PluginManager::getInstance()->getOculusPlatformPlugin();
    return (oculusPlatformPlugin && oculusPlatformPlugin->isRunning());
}

QString LoginDialog::oculusUserID() const {
    if (auto oculusPlatformPlugin = PluginManager::getInstance()->getOculusPlatformPlugin()) {
        return oculusPlatformPlugin->getOculusUserID();
    }
    return "";
}

void LoginDialog::dismissLoginDialog() {
    QAction* loginAction = Menu::getInstance()->getActionForOption(MenuOption::Login);
    Q_CHECK_PTR(loginAction);
    loginAction->setEnabled(true);

    emit dismissedLoginDialog();
}

void LoginDialog::login(const QString& username, const QString& password) const {
    qDebug() << "Attempting to login" << username;
    DependencyManager::get<AccountManager>()->requestAccessToken(username, password);
}

void LoginDialog::loginDomain(const QString& username, const QString& password) const {
    qDebug() << "Attempting to login" << username << "into a domain";
    DependencyManager::get<DomainAccountManager>()->requestAccessToken(username, password);
}

void LoginDialog::loginThroughOculus() {
   qDebug() << "Attempting to login through Oculus";
    if (auto oculusPlatformPlugin = PluginManager::getInstance()->getOculusPlatformPlugin()) {
        oculusPlatformPlugin->requestNonceAndUserID([] (QString nonce, QString oculusID) {
            DependencyManager::get<AccountManager>()->requestAccessTokenWithOculus(nonce, oculusID);
        });
    }
}

void LoginDialog::linkOculus() {
    qDebug() << "Attempting to link Oculus account";
    if (auto oculusPlatformPlugin = PluginManager::getInstance()->getOculusPlatformPlugin()) {
        oculusPlatformPlugin->requestNonceAndUserID([this] (QString nonce, QString oculusID) {
            if (nonce.isEmpty() || oculusID.isEmpty()) {
                emit handleLoginFailed();
                return;
            }

            JSONCallbackParameters callbackParams;
            callbackParams.callbackReceiver = this;
            callbackParams.jsonCallbackMethod = "linkCompleted";
            callbackParams.errorCallbackMethod = "linkFailed";
            const QString LINK_OCULUS_PATH = "/api/v1/user/oculus/link";

            QJsonObject payload;
            payload["oculus_nonce"] = nonce;
            payload["oculus_id"] = oculusID;

            auto accountManager = DependencyManager::get<AccountManager>();
            accountManager->sendRequest(LINK_OCULUS_PATH, AccountManagerAuth::Required,
                                        QNetworkAccessManager::PostOperation, callbackParams,
                                        QJsonDocument(payload).toJson());
        });
    }
}

void LoginDialog::createAccountFromOculus(QString email, QString username, QString password) {
    qDebug() << "Attempting to create account from Oculus info";
    if (auto oculusPlatformPlugin = PluginManager::getInstance()->getOculusPlatformPlugin()) {
        oculusPlatformPlugin->requestNonceAndUserID([this, email, username, password] (QString nonce, QString oculusID) {
            if (nonce.isEmpty() || oculusID.isEmpty()) {
                emit handleLoginFailed();
                return;
            }

            JSONCallbackParameters callbackParams;
            callbackParams.callbackReceiver = this;
            callbackParams.jsonCallbackMethod = "createCompleted";
            callbackParams.errorCallbackMethod = "createFailed";

            const QString CREATE_ACCOUNT_FROM_OCULUS_PATH = "/api/v1/user/oculus/create";

            QJsonObject payload;
            payload["oculus_nonce"] = nonce;
            payload["oculus_id"] = oculusID;
            if (!email.isEmpty()) {
                payload["email"] = email;
            }
            if (!username.isEmpty()) {
                payload["username"] = username;
            }
            if (!password.isEmpty()) {
                payload["password"] = password;
            }

            auto accountManager = DependencyManager::get<AccountManager>();
            accountManager->sendRequest(CREATE_ACCOUNT_FROM_OCULUS_PATH, AccountManagerAuth::None,
                                        QNetworkAccessManager::PostOperation, callbackParams,
                                        QJsonDocument(payload).toJson());
            });
    }
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

            const QString LINK_STEAM_PATH = "/api/v1/user/steam/link";

            QJsonObject payload;
            payload["steam_auth_ticket"] = QJsonValue::fromVariant(QVariant(ticket));

            auto accountManager = DependencyManager::get<AccountManager>();
            accountManager->sendRequest(LINK_STEAM_PATH, AccountManagerAuth::Required,
                                        QNetworkAccessManager::PostOperation, callbackParams,
                                        QJsonDocument(payload).toJson());
        });
    }
}

void LoginDialog::createAccountFromSteam(QString username) {
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

            const QString CREATE_ACCOUNT_FROM_STEAM_PATH = "/api/v1/user/steam/create";

            QJsonObject payload;
            payload["steam_auth_ticket"] = QJsonValue::fromVariant(QVariant(ticket));
            if (!username.isEmpty()) {
                payload["username"] = username;
            }

            auto accountManager = DependencyManager::get<AccountManager>();
            accountManager->sendRequest(CREATE_ACCOUNT_FROM_STEAM_PATH, AccountManagerAuth::None,
                                        QNetworkAccessManager::PostOperation, callbackParams,
                                        QJsonDocument(payload).toJson());
        });
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
    if (isOculusRunning()) {
        auto replyData = reply->readAll();
        QJsonParseError parseError;
        auto doc = QJsonDocument::fromJson(replyData, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            emit handleCreateFailed(reply->errorString());
            return;
        }
        auto data = doc["data"];
        auto error = data["error"];
        auto oculusError = data["oculus"];
        auto user = error["username"].toArray();
        auto uid = error["uid"].toArray();
        auto email = error["email"].toArray();
        auto password = error["password"].toArray();
        QString reply;
        if (uid[0].isString()) {
            emit handleCreateFailed("Oculus ID " + uid[0].toString() + ".");
            return;
        }
        if (user[0].isString()) {
            reply = "Username " + user[0].toString() + ".";
        }
        if (email[0].isString()) {
            reply.append((!reply.isEmpty()) ? "\n" : "");
            reply.append("Email " + email[0].toString() + ".");
        }
        if (password[0].isString()) {
            reply.append((!reply.isEmpty()) ? "\n" : "");
            reply.append("Password " + password[0].toString() + ".");
        }
        if (!oculusError.isNull() && !oculusError.isUndefined()) {
            emit handleCreateFailed("Could not verify token with Oculus. Please try again.");
            return;
        } else {
            emit handleCreateFailed(reply);
            return;
        }
    }
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

    qDebug() << "Sending a request to create an account for" << username;

    auto accountManager = DependencyManager::get<AccountManager>();
    accountManager->sendRequest(API_SIGNUP_PATH, AccountManagerAuth::None,
                                QNetworkAccessManager::PostOperation, callbackParams,
                                QJsonDocument(payload).toJson());
}

void LoginDialog::signupCompleted(QNetworkReply* reply) {
    emit handleSignupCompleted();
}

bool LoginDialog::getLoginDialogPoppedUp() const {
    return qApp->getLoginDialogPoppedUp();
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

bool LoginDialog::getDomainLoginRequested() const {
    return DependencyManager::get<DialogsManager>()->getIsDomainLogin();
}

QString LoginDialog::getDomainLoginDomain() const {
    return DependencyManager::get<DialogsManager>()->getDomainLoginDomain();
}
