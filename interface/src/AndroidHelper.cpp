//
//  AndroidHelper.cpp
//  interface/src
//
//  Created by Gabriel Calero & Cristian Duarte on 3/30/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "AndroidHelper.h"
#include <QDebug>
#include <AccountManager.h>
#include <AudioClient.h>
#include <src/ui/LoginDialog.h>
#include "Application.h"
#include "Constants.h"

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<Application*>(QCoreApplication::instance()))

AndroidHelper::AndroidHelper() {
    qRegisterMetaType<QAudio::Mode>("QAudio::Mode");
}

AndroidHelper::~AndroidHelper() {
}

void AndroidHelper::requestActivity(const QString &activityName, const bool backToScene, QMap<QString, QString> args) {
    emit androidActivityRequested(activityName, backToScene, args);
}

void AndroidHelper::notifyLoadComplete() {
    emit qtAppLoadComplete();
}

void AndroidHelper::notifyEnterForeground() {
    emit enterForeground();
}

void AndroidHelper::notifyBeforeEnterBackground() {
    emit beforeEnterBackground();
}

void AndroidHelper::notifyEnterBackground() {
    emit enterBackground();
}

void AndroidHelper::performHapticFeedback(int duration) {
    emit hapticFeedbackRequested(duration);
}

void AndroidHelper::showLoginDialog(QUrl url) {
    QMap<QString, QString> args;
    args["url"] = url.toString();
    emit androidActivityRequested("Login", true, args);
}

void AndroidHelper::processURL(const QString &url) {
    if (qApp->canAcceptURL(url)) {
        qApp->acceptURL(url);
    }
}

void AndroidHelper::notifyHeadsetOn(bool pluggedIn) {
#if defined (Q_OS_ANDROID)
    auto audioClient = DependencyManager::get<AudioClient>();
    if (audioClient) {
        QMetaObject::invokeMethod(audioClient.data(), "setHeadsetPluggedIn", Q_ARG(bool, pluggedIn));
    }
#endif
}

void AndroidHelper::signup(QString email, QString username, QString password) {
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

    auto accountManager = DependencyManager::get<AccountManager>();

    accountManager->sendRequest(API_SIGNUP_PATH, AccountManagerAuth::None,
                                QNetworkAccessManager::PostOperation, callbackParams,
                                QJsonDocument(payload).toJson());
}

void AndroidHelper::signupCompleted(QNetworkReply* reply) {
    emit handleSignupCompleted();
}

QString AndroidHelper::errorStringFromAPIObject(const QJsonValue& apiObject) {
    if (apiObject.isArray()) {
        return apiObject.toArray()[0].toString();
    } else if (apiObject.isString()) {
        return apiObject.toString();
    } else {
        return "is invalid";
    }
}

void AndroidHelper::signupFailed(QNetworkReply* reply) {
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
