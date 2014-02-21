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
    
    bool isEmpty() const { return jsonCallbackReceiver == NULL && errorCallbackReceiver == NULL; }
    
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
                              QNetworkAccessManager::Operation operation,
                              const JSONCallbackParameters& callbackParams = JSONCallbackParameters(),
                              const QByteArray& dataByteArray = QByteArray());
    
    void setRootURL(const QUrl& rootURL);
    
    bool hasValidAccessToken();
    bool checkAndSignalForAccessToken();
    
    void requestAccessToken(const QString& login, const QString& password);
    
    QString getUsername() const { return _accounts[_rootURL].getUsername(); }
    
public slots:
    void requestFinished();
    void requestError(QNetworkReply::NetworkError error);
signals:
    void authenticationRequired();
    void receivedAccessToken(const QUrl& rootURL);
    void usernameChanged(const QString& username);
private slots:
    void passSuccessToCallback();
    void passErrorToCallback(QNetworkReply::NetworkError errorCode);
private:
    AccountManager();
    AccountManager(AccountManager const& other); // not implemented
    void operator=(AccountManager const& other); // not implemented
    
    Q_INVOKABLE void invokedRequest(const QString& path, QNetworkAccessManager::Operation operation,
                                    const JSONCallbackParameters& callbackParams, const QByteArray& dataByteArray);
    
    QUrl _rootURL;
    QString _username;
    QNetworkAccessManager _networkAccessManager;
    QMap<QNetworkReply*, JSONCallbackParameters> _pendingCallbackMap;
    
    QMap<QUrl, DataServerAccountInfo> _accounts;
};

#endif /* defined(__hifi__AccountManager__) */
