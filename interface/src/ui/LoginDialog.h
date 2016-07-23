//
//  LoginDialog.h
//  interface/src/ui
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_LoginDialog_h
#define hifi_LoginDialog_h

#include <OffscreenQmlDialog.h>

class QNetworkReply;

class LoginDialog : public OffscreenQmlDialog {
    Q_OBJECT
    HIFI_QML_DECL

public:
    static void toggleAction();

    LoginDialog(QQuickItem* parent = nullptr);

signals:
    void handleLoginCompleted();
    void handleLoginFailed();

    void handleLinkCompleted();
    void handleLinkFailed(QString error);

    void handleCreateCompleted();
    void handleCreateFailed(QString error);

public slots:
    void linkCompleted(QNetworkReply& reply);
    void linkFailed(QNetworkReply& reply);

    void createCompleted(QNetworkReply& reply);
    void createFailed(QNetworkReply& reply);

protected slots:
    Q_INVOKABLE bool isSteamRunning();

    Q_INVOKABLE void login(const QString& username, const QString& password);
    Q_INVOKABLE void loginThroughSteam();
    Q_INVOKABLE void linkSteam();
    Q_INVOKABLE void createAccountFromStream(QString username = QString());

    Q_INVOKABLE void openUrl(const QString& url);
    Q_INVOKABLE void sendRecoveryEmail(const QString& email);

};

#endif // hifi_LoginDialog_h
