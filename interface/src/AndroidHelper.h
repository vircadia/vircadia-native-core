//
//  AndroidHelper.h
//  interface/src
//
//  Created by Gabriel Calero & Cristian Duarte on 3/30/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Android_Helper_h
#define hifi_Android_Helper_h

#include <QObject>
#include <QMap>
#include <QUrl>

#include <QNetworkReply>
#include <QtCore/QEventLoop>

class AndroidHelper : public QObject {
    Q_OBJECT
public:
    static AndroidHelper& instance() {
            static AndroidHelper instance;
            return instance;
    }
    void requestActivity(const QString &activityName, const bool backToScene, QMap<QString, QString> args = QMap<QString, QString>());
    void notifyLoadComplete();
    void notifyEnterForeground();
    void notifyBeforeEnterBackground();
    void notifyEnterBackground();

    void performHapticFeedback(int duration);
    void processURL(const QString &url);
    void notifyHeadsetOn(bool pluggedIn);
    void muteMic();

    AndroidHelper(AndroidHelper const&)  = delete;
    void operator=(AndroidHelper const&) = delete;

    void signup(QString email, QString username, QString password);

public slots:
    void showLoginDialog(QUrl url);
    void signupCompleted(QNetworkReply* reply);
    void signupFailed(QNetworkReply* reply);
signals:
    void androidActivityRequested(const QString &activityName, const bool backToScene, QMap<QString, QString> args = QMap<QString, QString>());
    void qtAppLoadComplete();
    void enterForeground();
    void beforeEnterBackground();
    void enterBackground();

    void hapticFeedbackRequested(int duration);

    void handleSignupCompleted();
    void handleSignupFailed(QString errorString);

private:
    AndroidHelper();
    ~AndroidHelper();

    QString errorStringFromAPIObject(const QJsonValue& apiObject);
};

#endif
