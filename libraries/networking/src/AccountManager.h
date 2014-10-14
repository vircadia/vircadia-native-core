//
//  AccountManager.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AccountManager_h
#define hifi_AccountManager_h

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>

#include "NetworkAccessManager.h"

#include "DataServerAccountInfo.h"

class JSONCallbackParameters {
public:
    JSONCallbackParameters(QObject* jsonCallbackReceiver = NULL, const QString& jsonCallbackMethod = QString(),
                           QObject* errorCallbackReceiver = NULL, const QString& errorCallbackMethod = QString(),
                           QObject* updateReceiver = NULL, const QString& updateSlot = QString());

    bool isEmpty() const { return !jsonCallbackReceiver && !errorCallbackReceiver; }

    QObject* jsonCallbackReceiver;
    QString jsonCallbackMethod;
    QObject* errorCallbackReceiver;
    QString errorCallbackMethod;
    QObject* updateReciever;
    QString updateSlot;
};

const QByteArray ACCESS_TOKEN_AUTHORIZATION_HEADER = "Authorization";

class AccountManager : public QObject {
    Q_OBJECT
public:
    static AccountManager& getInstance(bool forceReset = false);

    void authenticatedRequest(const QString& path,
                              QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
                              const JSONCallbackParameters& callbackParams = JSONCallbackParameters(),
                              const QByteArray& dataByteArray = QByteArray(),
                              QHttpMultiPart* dataMultiPart = NULL);
    
    void unauthenticatedRequest(const QString& path,
                                QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
                                const JSONCallbackParameters& callbackParams = JSONCallbackParameters(),
                                const QByteArray& dataByteArray = QByteArray(),
                                QHttpMultiPart* dataMultiPart = NULL);

    const QUrl& getAuthURL() const { return _authURL; }
    void setAuthURL(const QUrl& authURL);
    bool hasAuthEndpoint() { return !_authURL.isEmpty(); }
    
    void disableSettingsFilePersistence() { _shouldPersistToSettingsFile = false; }

    bool isLoggedIn() { return !_authURL.isEmpty() && hasValidAccessToken(); }
    bool hasValidAccessToken();
    Q_INVOKABLE bool checkAndSignalForAccessToken();
    void setAccessTokenForCurrentAuthURL(const QString& accessToken);

    void requestAccessToken(const QString& login, const QString& password);
    void requestProfile();

    DataServerAccountInfo& getAccountInfo() { return _accountInfo; }

public slots:
    void requestAccessTokenFinished();
    void requestProfileFinished();
    void requestAccessTokenError(QNetworkReply::NetworkError error);
    void requestProfileError(QNetworkReply::NetworkError error);
    void logout();
    void updateBalance();
    void accountInfoBalanceChanged(qint64 newBalance);
signals:
    void authRequired();
    void authEndpointChanged();
    void usernameChanged(const QString& username);
    void profileChanged();
    void loginComplete(const QUrl& authURL);
    void loginFailed();
    void logoutComplete();
    void balanceChanged(qint64 newBalance);
private slots:
    void processReply();
    void handleKeypairGenerationError();
    void processGeneratedKeypair(const QByteArray& publicKey, const QByteArray& privateKey);
private:
    AccountManager();
    AccountManager(AccountManager const& other); // not implemented
    void operator=(AccountManager const& other); // not implemented
    
    void persistAccountToSettings();

    void passSuccessToCallback(QNetworkReply* reply);
    void passErrorToCallback(QNetworkReply* reply);
    
    void generateNewKeypair();

    Q_INVOKABLE void invokedRequest(const QString& path,
                                    bool requiresAuthentication,
                                    QNetworkAccessManager::Operation operation,
                                    const JSONCallbackParameters& callbackParams,
                                    const QByteArray& dataByteArray,
                                    QHttpMultiPart* dataMultiPart);

    QUrl _authURL;
    QMap<QNetworkReply*, JSONCallbackParameters> _pendingCallbackMap;

    DataServerAccountInfo _accountInfo;
    bool _shouldPersistToSettingsFile;
};

#endif // hifi_AccountManager_h
