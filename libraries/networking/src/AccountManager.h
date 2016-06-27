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
#include <QUrlQuery>

#include "NetworkAccessManager.h"

#include "DataServerAccountInfo.h"
#include "SharedUtil.h"

#include <DependencyManager.h>

class JSONCallbackParameters {
public:
    JSONCallbackParameters(QObject* jsonCallbackReceiver = nullptr, const QString& jsonCallbackMethod = QString(),
                           QObject* errorCallbackReceiver = nullptr, const QString& errorCallbackMethod = QString(),
                           QObject* updateReceiver = nullptr, const QString& updateSlot = QString());

    bool isEmpty() const { return !jsonCallbackReceiver && !errorCallbackReceiver; }

    QObject* jsonCallbackReceiver;
    QString jsonCallbackMethod;
    QObject* errorCallbackReceiver;
    QString errorCallbackMethod;
    QObject* updateReciever;
    QString updateSlot;
};

namespace AccountManagerAuth {
    enum Type {
        None,
        Required,
        Optional
    };
}

Q_DECLARE_METATYPE(AccountManagerAuth::Type);

const QByteArray ACCESS_TOKEN_AUTHORIZATION_HEADER = "Authorization";

using UserAgentGetter = std::function<QString()>;

const auto DEFAULT_USER_AGENT_GETTER = []() -> QString { return HIGH_FIDELITY_USER_AGENT; };

class AccountManager : public QObject, public Dependency {
    Q_OBJECT
public:
    AccountManager(UserAgentGetter userAgentGetter = DEFAULT_USER_AGENT_GETTER);

    Q_INVOKABLE void sendRequest(const QString& path,
                                 AccountManagerAuth::Type authType,
                                 QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
                                 const JSONCallbackParameters& callbackParams = JSONCallbackParameters(),
                                 const QByteArray& dataByteArray = QByteArray(),
                                 QHttpMultiPart* dataMultiPart = NULL,
                                 const QVariantMap& propertyMap = QVariantMap(),
                                 QUrlQuery query = QUrlQuery());

    void setIsAgent(bool isAgent) { _isAgent = isAgent; }

    const QUrl& getAuthURL() const { return _authURL; }
    void setAuthURL(const QUrl& authURL);
    bool hasAuthEndpoint() { return !_authURL.isEmpty(); }

    bool isLoggedIn() { return !_authURL.isEmpty() && hasValidAccessToken(); }
    bool hasValidAccessToken();
    Q_INVOKABLE bool checkAndSignalForAccessToken();
    void setAccessTokenForCurrentAuthURL(const QString& accessToken);

    void requestProfile();

    DataServerAccountInfo& getAccountInfo() { return _accountInfo; }

    static QJsonObject dataObjectFromResponse(QNetworkReply& requestReply);

    QUuid getSessionID() const { return _sessionID; }
    void setSessionID(const QUuid& sessionID) { _sessionID = sessionID; }

    void setTemporaryDomain(const QUuid& domainID, const QString& key);
    const QString& getTemporaryDomainKey(const QUuid& domainID) { return _accountInfo.getTemporaryDomainKey(domainID); }

public slots:
    void requestAccessToken(const QString& login, const QString& password);

    void requestAccessTokenFinished();
    void requestProfileFinished();
    void requestAccessTokenError(QNetworkReply::NetworkError error);
    void requestProfileError(QNetworkReply::NetworkError error);
    void logout();
    void generateNewUserKeypair() { generateNewKeypair(); }
    void generateNewDomainKeypair(const QUuid& domainID) { generateNewKeypair(false, domainID); }

signals:
    void authRequired();
    void authEndpointChanged();
    void usernameChanged(const QString& username);
    void profileChanged();
    void loginComplete(const QUrl& authURL);
    void loginFailed();
    void logoutComplete();
    void newKeypair();

private slots:
    void processReply();
    void handleKeypairGenerationError();
    void processGeneratedKeypair();
    void publicKeyUploadSucceeded(QNetworkReply& reply);
    void publicKeyUploadFailed(QNetworkReply& reply);
    void generateNewKeypair(bool isUserKeypair = true, const QUuid& domainID = QUuid());

private:
    AccountManager(AccountManager const& other) = delete;
    void operator=(AccountManager const& other) = delete;

    void persistAccountToFile();
    void removeAccountFromFile();

    void passSuccessToCallback(QNetworkReply* reply);
    void passErrorToCallback(QNetworkReply* reply);

    UserAgentGetter _userAgentGetter;

    QUrl _authURL;
    
    QMap<QNetworkReply*, JSONCallbackParameters> _pendingCallbackMap;

    DataServerAccountInfo _accountInfo;
    bool _isAgent { false };

    bool _isWaitingForKeypairResponse { false };
    QByteArray _pendingPrivateKey;

    QUuid _sessionID { QUuid::createUuid() };
};

#endif // hifi_AccountManager_h
