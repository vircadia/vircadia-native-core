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


struct DomainAccountDetails {
    QUrl domainURL;
    QUrl authURL;
    QString clientID;
    QString username;
    QString accessToken;
    QString refreshToken;
    QString authedDomainName;
};


class DomainAccountManager : public QObject, public Dependency {
    Q_OBJECT
public:
    DomainAccountManager();

    void setDomainURL(const QUrl& domainURL);
    void setAuthURL(const QUrl& authURL);
    void setClientID(const QString& clientID) { _currentAuth.clientID = clientID; }

    const QString& getUsername() { return _currentAuth.username; }
    const QString& getAccessToken() { return _currentAuth.accessToken; }
    const QString& getRefreshToken() { return _currentAuth.refreshToken; }
    const QString& getAuthedDomainName() { return _currentAuth.authedDomainName; }

    bool hasLogIn();
    bool isLoggedIn();

    Q_INVOKABLE bool checkAndSignalForAccessToken();

public slots:
    void requestAccessToken(const QString& username, const QString& password);
    void requestAccessTokenFinished();

signals:
    void hasLogInChanged(bool hasLogIn);
    void authRequired(const QString& domain);
    void loginComplete();
    void loginFailed();
    void logoutComplete();
    void newTokens();

private:
    bool hasValidAccessToken();
    bool accessTokenIsExpired();
    void setTokensFromJSON(const QJsonObject&, const QUrl& url);
    void sendInterfaceAccessTokenToServer();

    DomainAccountDetails _currentAuth;
    QHash<QUrl, DomainAccountDetails> _knownAuths;  // <domainURL, DomainAccountDetails>
};

#endif  // hifi_DomainAccountManager_h
