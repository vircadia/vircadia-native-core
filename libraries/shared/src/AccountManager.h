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

#include "DataServerAccountInfo.h"

class JSONCallbackParameters {
public:
    JSONCallbackParameters() :
        jsonCallbackReceiver(NULL), jsonCallbackMethod(),
        errorCallbackReceiver(NULL), errorCallbackMethod() {};
    
    bool isEmpty() const { return !jsonCallbackReceiver && !errorCallbackReceiver; }
    
    QObject* jsonCallbackReceiver;
    QString jsonCallbackMethod;
    QObject* errorCallbackReceiver;
    QString errorCallbackMethod;
};

class AccountManager : public QObject {
    Q_OBJECT
public:
    static AccountManager& getInstance();
    
    void authenticatedRequest(const QString& path,
                              QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
                              const JSONCallbackParameters& callbackParams = JSONCallbackParameters(),
                              const QByteArray& dataByteArray = QByteArray(),
                              QHttpMultiPart* dataMultiPart = NULL);
    
    const QUrl& getAuthURL() const { return _authURL; }
    void setAuthURL(const QUrl& authURL);
    bool hasAuthEndpoint() { return !_authURL.isEmpty(); }
    
    bool isLoggedIn() { return !_authURL.isEmpty() && hasValidAccessToken(); }
    bool hasValidAccessToken();
    Q_INVOKABLE bool checkAndSignalForAccessToken();
    
    void requestAccessToken(const QString& login, const QString& password);
    
    QString getUsername() const { return _accountInfo.getUsername(); }

    const QString& getXMPPPassword() const { return _accountInfo.getXMPPPassword(); }
    
    void destroy() { delete _networkAccessManager; }
    
public slots:
    void requestFinished();
    void requestError(QNetworkReply::NetworkError error);
    void logout();
signals:
    void authRequired();
    void authEndpointChanged();
    void usernameChanged(const QString& username);
    void accessTokenChanged();
    void loginComplete(const QUrl& authURL);
    void logoutComplete();
private slots:
    void passSuccessToCallback();
    void passErrorToCallback(QNetworkReply::NetworkError errorCode);
private:
    AccountManager();
    AccountManager(AccountManager const& other); // not implemented
    void operator=(AccountManager const& other); // not implemented
    
    Q_INVOKABLE void invokedRequest(const QString& path, QNetworkAccessManager::Operation operation,
                                    const JSONCallbackParameters& callbackParams,
                                    const QByteArray& dataByteArray,
                                    QHttpMultiPart* dataMultiPart);
    
    QUrl _authURL;
    QNetworkAccessManager* _networkAccessManager;
    QMap<QNetworkReply*, JSONCallbackParameters> _pendingCallbackMap;
    
    DataServerAccountInfo _accountInfo;
};

#endif /* defined(__hifi__AccountManager__) */
