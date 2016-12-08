//
//  SignupDialog.cpp
//  interface/src/ui
//
//  Created by Stephen Birarda on December 7 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SignupDialog.h"

#include <QtCore/QJsonDocument>

#include <AccountManager.h>

HIFI_QML_DEF(SignupDialog)

SignupDialog::SignupDialog(QQuickItem *parent) : OffscreenQmlDialog(parent) {
    
}

void SignupDialog::signup(const QString& username, const QString& password) {
    
    JSONCallbackParameters callbackParams;
    callbackParams.jsonCallbackReceiver = this;
    callbackParams.jsonCallbackMethod = "signupCompleted";
    callbackParams.errorCallbackReceiver = this;
    callbackParams.errorCallbackMethod = "signupFailed";
    
    QJsonObject payload;
    payload.insert("username", username);
    payload.insert("password", password);
    
    static const QString API_SIGNUP_PATH = "api/v1/users";
    
    qDebug() << "Sending a request to create an account for" << username;
    
    auto accountManager = DependencyManager::get<AccountManager>();
    accountManager->sendRequest(API_SIGNUP_PATH, AccountManagerAuth::None,
                                QNetworkAccessManager::PostOperation, callbackParams,
                                QJsonDocument(payload).toJson());
}

void SignupDialog::signupCompleted(QNetworkReply& reply) {
    emit handleSignupCompleted();
}

void SignupDialog::signupFailed(QNetworkReply& reply) {
    
}
