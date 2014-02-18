//
//  AccountManager.h
//  hifi
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AccountManager__
#define __hifi__AccountManager__

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>

#include "OAuthAccessToken.h"

class AccountManager : public QObject {
    Q_OBJECT
public:
    static AccountManager& getInstance();
    
    bool hasValidAccessTokenForRootURL(const QUrl& rootURL);
    
    const QString& getUsername() const { return _username; }
    void setUsername(const QString& username) { _username = username; }
    
    void setNetworkAccessManager(QNetworkAccessManager* networkAccessManager) { _networkAccessManager = networkAccessManager; }
signals:
    void authenticationRequiredForRootURL(const QUrl& rootURL);
private:
    AccountManager();
    AccountManager(AccountManager const& other); // not implemented
    void operator=(AccountManager const& other); // not implemented
    
    QString _username;
    QMap<QUrl, OAuthAccessToken> _accessTokens;
    QNetworkAccessManager* _networkAccessManager;
};

#endif /* defined(__hifi__AccountManager__) */
