//
//  SignupDialog.h
//  interface/src/ui
//
//  Created by Stephen Birarda on December 7 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_SignupDialog_h
#define hifi_SignupDialog_h

#include <OffscreenQmlDialog.h>

class QNetworkReply;

class SignupDialog : public OffscreenQmlDialog {
    Q_OBJECT
    HIFI_QML_DECL
    
public:
    SignupDialog(QQuickItem* parent = nullptr);
    
public slots:
    void signupCompleted(QNetworkReply& reply);
    void signupFailed(QNetworkReply& reply);
    
signals:
    void handleSignupCompleted();
    void handleSignupFailed(QString error);
    
protected slots:
    Q_INVOKABLE void signup(const QString& username, const QString& password);
};

#endif // hifi_SignupDialog_h
