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
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>
#include <QUrlQuery>

#include <DependencyManager.h>

#include "AccountSettings.h"
#include "DataServerAccountInfo.h"
#include "NetworkingConstants.h"
#include "MetaverseAPI.h"
#include "NetworkAccessManager.h"
#include <SharedUtil.h>

class JSONCallbackParameters {
public:
    JSONCallbackParameters(QObject* callbackReceiver = nullptr,
        const QString& jsonCallbackMethod = QString(),
        const QString& errorCallbackMethod = QString(),
        const QJsonObject& callbackData = QJsonObject());

    bool isEmpty() const { return !callbackReceiver; }

    QObject* callbackReceiver;
    QString jsonCallbackMethod;
    QString errorCallbackMethod;
    QJsonObject callbackData;
};

namespace AccountManagerAuth {
enum Type {
    None,
    Required,
    Optional,
};
}

Q_DECLARE_METATYPE(AccountManagerAuth::Type);

const QByteArray ACCESS_TOKEN_AUTHORIZATION_HEADER = "Authorization";
const auto METAVERSE_SESSION_ID_HEADER = QString("HFM-SessionID").toLocal8Bit();

using UserAgentGetter = std::function<QString()>;

const auto DEFAULT_USER_AGENT_GETTER = []() -> QString { return NetworkingConstants::VIRCADIA_USER_AGENT; };

class AccountManager : public QObject, public Dependency {
    Q_OBJECT
public:
    AccountManager(bool accountSettingsEnabled = false, UserAgentGetter userAgentGetter = DEFAULT_USER_AGENT_GETTER);

    QNetworkRequest createRequest(QString path, AccountManagerAuth::Type authType);
    Q_INVOKABLE void sendRequest(const QString& path,
                                 AccountManagerAuth::Type authType,
                                 QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
                                 const JSONCallbackParameters& callbackParams = JSONCallbackParameters(),
                                 const QByteArray& dataByteArray = QByteArray(),
                                 QHttpMultiPart* dataMultiPart = NULL,
                                 const QVariantMap& propertyMap = QVariantMap());

    void setIsAgent(bool isAgent) { _isAgent = isAgent; }

    const QUrl& getAuthURL() const { return _authURL; }
    void setAuthURL(const QUrl& authURL);
    bool hasAuthEndpoint() { return !_authURL.isEmpty(); }

    bool isLoggedIn() { return !_authURL.isEmpty() && hasValidAccessToken(); }
    bool hasValidAccessToken();
    bool needsToRefreshToken();
    Q_INVOKABLE bool checkAndSignalForAccessToken();
    void setAccessTokenForCurrentAuthURL(const QString& accessToken);
    bool hasKeyPair() const;

    void requestProfile();

    DataServerAccountInfo& getAccountInfo() { return _accountInfo; }
    void setAccountInfo(const DataServerAccountInfo& newAccountInfo);

    static QJsonObject dataObjectFromResponse(QNetworkReply* requestReply);

    QUuid getSessionID() const { return _sessionID; }
    void setSessionID(const QUuid& sessionID);

    void setTemporaryDomain(const QUuid& domainID, const QString& key);
    const QString& getTemporaryDomainKey(const QUuid& domainID) { return _accountInfo.getTemporaryDomainKey(domainID); }

    QUrl getMetaverseServerURL() { return MetaverseAPI::getCurrentMetaverseServerURL(); }
    QString getMetaverseServerURLPath(bool appendForwardSlash = false) {
        return MetaverseAPI::getCurrentMetaverseServerURLPath(appendForwardSlash);
    }

    void removeAccountFromFile();

    bool getLimitedCommerce() { return _limitedCommerce; }
    void setLimitedCommerce(bool isLimited);

    void setAccessTokens(const QString& response);
    void setConfigFileURL(const QString& fileURL) { _configFileURL = fileURL; }
    void saveLoginStatus(bool isLoggedIn);

    AccountSettings& getAccountSettings() { return _settings; }

public slots:
    void requestAccessToken(const QString& login, const QString& password);
    void requestAccessTokenWithSteam(QByteArray authSessionTicket);
    void requestAccessTokenWithOculus(const QString& nonce, const QString& oculusID);
    void requestAccessTokenWithAuthCode(const QString& authCode,
                                        const QString& clientId,
                                        const QString& clientSecret,
                                        const QString& redirectUri);
    void refreshAccessToken();

    void requestAccessTokenFinished();
    void refreshAccessTokenFinished();
    void requestProfileFinished();
    void refreshAccessTokenError(QNetworkReply::NetworkError error);
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
    void limitedCommerceChanged();
    void accountSettingsLoaded();

private slots:
    void handleKeypairGenerationError();
    void processGeneratedKeypair(QByteArray publicKey, QByteArray privateKey);
    void uploadPublicKey();
    void publicKeyUploadSucceeded(QNetworkReply* reply);
    void publicKeyUploadFailed(QNetworkReply* reply);
    void generateNewKeypair(bool isUserKeypair = true, const QUuid& domainID = QUuid());

    void requestAccountSettings();
    void requestAccountSettingsFinished();
    void requestAccountSettingsError(QNetworkReply::NetworkError error);
    void postAccountSettings();
    void postAccountSettingsFinished();
    void postAccountSettingsError(QNetworkReply::NetworkError error);

private:
    Q_DISABLE_COPY(AccountManager);

    void persistAccountToFile();

    void passSuccessToCallback(QNetworkReply* reply);
    void passErrorToCallback(QNetworkReply* reply);

    UserAgentGetter _userAgentGetter;

    QUrl _authURL;

    DataServerAccountInfo _accountInfo;
    bool _isWaitingForTokenRefresh { false };
    bool _isAgent { false };

    bool _isWaitingForKeypairResponse { false };
    QByteArray _pendingPrivateKey;
    QByteArray _pendingPublicKey;

    QUuid _sessionID { QUuid::createUuid() };

    bool _limitedCommerce { false };
    QString _configFileURL;

    bool _accountSettingsEnabled { false };
    AccountSettings _settings;
    quint64 _currentSyncTimestamp { 0 };
    quint64 _lastSuccessfulSyncTimestamp { 0 };
    int _numPullRetries { 0 };
    QTimer* _pullSettingsRetryTimer { nullptr };
    QTimer* _postSettingsTimer { nullptr };
};

#endif  // hifi_AccountManager_h
