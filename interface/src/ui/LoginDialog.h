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

class LoginDialog : public OffscreenQmlDialog {
    Q_OBJECT
    HIFI_QML_DECL

    Q_PROPERTY(QString statusText READ statusText WRITE setStatusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString rootUrl READ rootUrl)

public:
    static void toggleAction();

    LoginDialog(QQuickItem* parent = nullptr);

    void setStatusText(const QString& statusText);
    QString statusText() const;

    QString rootUrl() const;

signals:
    void statusTextChanged();

protected:
    void handleLoginCompleted(const QUrl& authURL);
    void handleLoginFailed();

    Q_INVOKABLE void login(const QString& username, const QString& password);
    Q_INVOKABLE void loginThroughSteam();
    Q_INVOKABLE void openUrl(const QString& url);
private:
    QString _statusText;
    const QString _rootUrl;
};

#endif // hifi_LoginDialog_h
