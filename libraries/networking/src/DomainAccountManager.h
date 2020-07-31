//
//  DomainAccountManager.h
//  libraries/networking/src
//
//  Created by David Rowe on 23 Jul 2020.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DomainAccountManager_h
#define hifi_DomainAccountManager_h

#include <QtCore/QObject>
#include <QtCore/QUrl>

#include <DependencyManager.h>


class DomainAccountManager : public QObject, public Dependency {
    Q_OBJECT
public:
    DomainAccountManager();

    void setAuthURL(const QUrl& authURL);

    Q_INVOKABLE bool checkAndSignalForAccessToken();

public slots:
    void requestAccessToken(const QString& login, const QString& password);
    
    void requestAccessTokenFinished();
signals:
    void authRequired();
    void loginComplete(const QUrl& authURL);
    void loginFailed();
    void logoutComplete();

private slots:

private:
    bool hasValidAccessToken();
    bool accessTokenIsExpired();
    void setAccessTokenFromJSON(const QJsonObject&);
    void sendInterfaceAccessTokenToServer();

    QUrl _authURL;
};

#endif  // hifi_DomainAccountManager_h
