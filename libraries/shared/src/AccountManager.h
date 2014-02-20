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
#include <QtNetwork/QNetworkReply>

#include "OAuthAccessToken.h"

namespace AuthenticatedRequestMethod {
    enum Method {
        GET,
        POST,
        PUT,
        DELETE
    };
}

class AccountManager : public QObject {
    Q_OBJECT
public:
    
    static AccountManager& getInstance();
    
    void authenticatedRequest(const QString& path,
                              AuthenticatedRequestMethod::Method method,
                              const QObject* successReceiver, const char* successMethod,
                              const QByteArray& dataByteArray = QByteArray(),
                              const QObject* errorReceiver = 0, const char* errorMethod = NULL);
    
    void setRootURL(const QUrl& rootURL) { _rootURL = rootURL; }
    
    bool hasValidAccessToken();
    bool checkAndSignalForAccessToken();
    
    void requestAccessToken(const QString& login, const QString& password);
    
    const QString& getUsername() const { return _username; }
    void setUsername(const QString& username) { _username = username; }
    
public slots:
    void requestFinished();
    void requestError(QNetworkReply::NetworkError error);
signals:
    void authenticationRequired();
    void receivedAccessToken(const QUrl& rootURL);
private:
    AccountManager();
    AccountManager(AccountManager const& other); // not implemented
    void operator=(AccountManager const& other); // not implemented
    
    QUrl _rootURL;
    QString _username;
    QNetworkAccessManager* _networkAccessManager;
    
    static QMap<QUrl, OAuthAccessToken> _accessTokens;
};

#endif /* defined(__hifi__AccountManager__) */
