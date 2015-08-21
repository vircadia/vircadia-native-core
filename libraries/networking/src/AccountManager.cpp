//
//  AccountManager.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <memory>

#include <QtCore/QDataStream>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMap>
#include <QtCore/QStringList>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QHttpMultiPart>
#include <QtNetwork/QNetworkRequest>
#include <qthread.h>

#include <SettingHandle.h>

#include "NodeList.h"
#include "udt/PacketHeaders.h"
#include "RSAKeypairGenerator.h"
#include "SharedUtil.h"

#include "AccountManager.h"
#include "NetworkLogging.h"

const bool VERBOSE_HTTP_REQUEST_DEBUGGING = false;

AccountManager& AccountManager::getInstance(bool forceReset) {
    static std::unique_ptr<AccountManager> sharedInstance(new AccountManager());
    
    if (forceReset) {
        sharedInstance.reset(new AccountManager());
    }
    
    return *sharedInstance;
}

Q_DECLARE_METATYPE(OAuthAccessToken)
Q_DECLARE_METATYPE(DataServerAccountInfo)
Q_DECLARE_METATYPE(QNetworkAccessManager::Operation)
Q_DECLARE_METATYPE(JSONCallbackParameters)

const QString ACCOUNTS_GROUP = "accounts";

JSONCallbackParameters::JSONCallbackParameters(QObject* jsonCallbackReceiver, const QString& jsonCallbackMethod,
                                               QObject* errorCallbackReceiver, const QString& errorCallbackMethod,
                                               QObject* updateReceiver, const QString& updateSlot) :
    jsonCallbackReceiver(jsonCallbackReceiver),
    jsonCallbackMethod(jsonCallbackMethod),
    errorCallbackReceiver(errorCallbackReceiver),
    errorCallbackMethod(errorCallbackMethod),
    updateReciever(updateReceiver),
    updateSlot(updateSlot)
{
}

AccountManager::AccountManager() :
    _authURL(),
    _pendingCallbackMap(),
    _accountInfo(),
    _shouldPersistToSettingsFile(true)
{
    qRegisterMetaType<OAuthAccessToken>("OAuthAccessToken");
    qRegisterMetaTypeStreamOperators<OAuthAccessToken>("OAuthAccessToken");

    qRegisterMetaType<DataServerAccountInfo>("DataServerAccountInfo");
    qRegisterMetaTypeStreamOperators<DataServerAccountInfo>("DataServerAccountInfo");

    qRegisterMetaType<QNetworkAccessManager::Operation>("QNetworkAccessManager::Operation");
    qRegisterMetaType<JSONCallbackParameters>("JSONCallbackParameters");
    
    qRegisterMetaType<QHttpMultiPart*>("QHttpMultiPart*");

    connect(&_accountInfo, &DataServerAccountInfo::balanceChanged, this, &AccountManager::accountInfoBalanceChanged);
    
    // once we have a profile in account manager make sure we generate a new keypair
    connect(this, &AccountManager::profileChanged, this, &AccountManager::generateNewKeypair);
}

const QString DOUBLE_SLASH_SUBSTITUTE = "slashslash";

void AccountManager::logout() {
    // a logout means we want to delete the DataServerAccountInfo we currently have for this URL, in-memory and in file
    _accountInfo = DataServerAccountInfo();

    emit balanceChanged(0);
    connect(&_accountInfo, &DataServerAccountInfo::balanceChanged, this, &AccountManager::accountInfoBalanceChanged);
    
    if (_shouldPersistToSettingsFile) {
        QString keyURLString(_authURL.toString().replace("//", DOUBLE_SLASH_SUBSTITUTE));
        QStringList path = QStringList() << ACCOUNTS_GROUP << keyURLString;
        Setting::Handle<DataServerAccountInfo>(path).remove();
        
        qCDebug(networking) << "Removed account info for" << _authURL << "from in-memory accounts and .ini file";
    } else {
        qCDebug(networking) << "Cleared data server account info in account manager.";
    }

    emit logoutComplete();
    // the username has changed to blank
    emit usernameChanged(QString());
}

void AccountManager::updateBalance() {
    if (hasValidAccessToken()) {
        // ask our auth endpoint for our balance
        JSONCallbackParameters callbackParameters;
        callbackParameters.jsonCallbackReceiver = &_accountInfo;
        callbackParameters.jsonCallbackMethod = "setBalanceFromJSON";

        sendRequest("/api/v1/wallets/mine", AccountManagerAuth::Required, QNetworkAccessManager::GetOperation, callbackParameters);
    }
}

void AccountManager::accountInfoBalanceChanged(qint64 newBalance) {
    emit balanceChanged(newBalance);
}

void AccountManager::setAuthURL(const QUrl& authURL) {
    if (_authURL != authURL) {
        _authURL = authURL;

        qCDebug(networking) << "AccountManager URL for authenticated requests has been changed to" << qPrintable(_authURL.toString());
        
        if (_shouldPersistToSettingsFile) {
            // check if there are existing access tokens to load from settings
            Settings settings;
            settings.beginGroup(ACCOUNTS_GROUP);
            
            foreach(const QString& key, settings.allKeys()) {
                // take a key copy to perform the double slash replacement
                QString keyCopy(key);
                QUrl keyURL(keyCopy.replace(DOUBLE_SLASH_SUBSTITUTE, "//"));
                
                if (keyURL == _authURL) {
                    // pull out the stored access token and store it in memory
                    _accountInfo = settings.value(key).value<DataServerAccountInfo>();
                    qCDebug(networking) << "Found a data-server access token for" << qPrintable(keyURL.toString());
                    
                    // profile info isn't guaranteed to be saved too
                    if (_accountInfo.hasProfile()) {
                        emit profileChanged();
                    } else {
                        requestProfile();
                    }
                }
            }
        }

        // tell listeners that the auth endpoint has changed
        emit authEndpointChanged();
    }
}

void AccountManager::sendRequest(const QString& path,
                                 AccountManagerAuth::Type authType,
                                 QNetworkAccessManager::Operation operation,
                                 const JSONCallbackParameters& callbackParams,
                                 const QByteArray& dataByteArray,
                                 QHttpMultiPart* dataMultiPart,
                                 const QVariantMap& propertyMap) {
    
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "sendRequest",
                                  Q_ARG(const QString&, path),
                                  Q_ARG(AccountManagerAuth::Type, AccountManagerAuth::Required),
                                  Q_ARG(QNetworkAccessManager::Operation, operation),
                                  Q_ARG(const JSONCallbackParameters&, callbackParams),
                                  Q_ARG(const QByteArray&, dataByteArray),
                                  Q_ARG(QHttpMultiPart*, dataMultiPart),
                                  Q_ARG(QVariantMap, propertyMap));
    }
    
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    
    QNetworkRequest networkRequest;
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
    
    QUrl requestURL = _authURL;
    
    if (path.startsWith("/")) {
        requestURL.setPath(path);
    } else {
        requestURL.setPath("/" + path);
    }
    
    if (authType != AccountManagerAuth::None ) {
        if (hasValidAccessToken()) {
            networkRequest.setRawHeader(ACCESS_TOKEN_AUTHORIZATION_HEADER,
                                        _accountInfo.getAccessToken().authorizationHeaderValue());
        } else {
            if (authType == AccountManagerAuth::Required) {
                qCDebug(networking) << "No valid access token present. Bailing on invoked request to"
                    << path << "that requires authentication";
                return;
            }
            
        }
    }
    
    networkRequest.setUrl(requestURL);
    
    if (VERBOSE_HTTP_REQUEST_DEBUGGING) {
        qCDebug(networking) << "Making a request to" << qPrintable(requestURL.toString());
        
        if (!dataByteArray.isEmpty()) {
            qCDebug(networking) << "The POST/PUT body -" << QString(dataByteArray);
        }
    }
    
    QNetworkReply* networkReply = NULL;
    
    switch (operation) {
        case QNetworkAccessManager::GetOperation:
            networkReply = networkAccessManager.get(networkRequest);
            break;
        case QNetworkAccessManager::PostOperation:
        case QNetworkAccessManager::PutOperation:
            if (dataMultiPart) {
                if (operation == QNetworkAccessManager::PostOperation) {
                    networkReply = networkAccessManager.post(networkRequest, dataMultiPart);
                } else {
                    networkReply = networkAccessManager.put(networkRequest, dataMultiPart);
                }
                
                // make sure dataMultiPart is destroyed when the reply is
                connect(networkReply, &QNetworkReply::destroyed, dataMultiPart, &QHttpMultiPart::deleteLater);
            } else {
                networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
                if (operation == QNetworkAccessManager::PostOperation) {
                    networkReply = networkAccessManager.post(networkRequest, dataByteArray);
                } else {
                    networkReply = networkAccessManager.put(networkRequest, dataByteArray);
                }
            }
            
            break;
        case QNetworkAccessManager::DeleteOperation:
            networkReply = networkAccessManager.sendCustomRequest(networkRequest, "DELETE");
            break;
        default:
            // other methods not yet handled
            break;
    }
    
    if (networkReply) {
        if (!propertyMap.isEmpty()) {
            // we have properties to set on the reply so the user can check them after
            foreach(const QString& propertyKey, propertyMap.keys()) {
                networkReply->setProperty(qPrintable(propertyKey), propertyMap.value(propertyKey));
            }
        }
        
        
        if (!callbackParams.isEmpty()) {
            // if we have information for a callback, insert the callbackParams into our local map
            _pendingCallbackMap.insert(networkReply, callbackParams);
            
            if (callbackParams.updateReciever && !callbackParams.updateSlot.isEmpty()) {
                callbackParams.updateReciever->connect(networkReply, SIGNAL(uploadProgress(qint64, qint64)),
                                                       callbackParams.updateSlot.toStdString().c_str());
            }
        }
        
        // if we ended up firing of a request, hook up to it now
        connect(networkReply, SIGNAL(finished()), SLOT(processReply()));
    }
}

void AccountManager::processReply() {
    QNetworkReply* requestReply = reinterpret_cast<QNetworkReply*>(sender());

    if (requestReply->error() == QNetworkReply::NoError) {
        passSuccessToCallback(requestReply);
    } else {
        passErrorToCallback(requestReply);
    }
    requestReply->deleteLater();
}

void AccountManager::passSuccessToCallback(QNetworkReply* requestReply) {
    JSONCallbackParameters callbackParams = _pendingCallbackMap.value(requestReply);

    if (callbackParams.jsonCallbackReceiver) {
        // invoke the right method on the callback receiver
        QMetaObject::invokeMethod(callbackParams.jsonCallbackReceiver, qPrintable(callbackParams.jsonCallbackMethod),
                                  Q_ARG(QNetworkReply&, *requestReply));

        // remove the related reply-callback group from the map
        _pendingCallbackMap.remove(requestReply);

    } else {
        if (VERBOSE_HTTP_REQUEST_DEBUGGING) {
            qCDebug(networking) << "Received JSON response from data-server that has no matching callback.";
            qCDebug(networking) << QJsonDocument::fromJson(requestReply->readAll());
        }
    }
}

void AccountManager::passErrorToCallback(QNetworkReply* requestReply) {
    JSONCallbackParameters callbackParams = _pendingCallbackMap.value(requestReply);

    if (callbackParams.errorCallbackReceiver) {
        // invoke the right method on the callback receiver
        QMetaObject::invokeMethod(callbackParams.errorCallbackReceiver, qPrintable(callbackParams.errorCallbackMethod),
                                  Q_ARG(QNetworkReply&, *requestReply));

        // remove the related reply-callback group from the map
        _pendingCallbackMap.remove(requestReply);
    } else {
        if (VERBOSE_HTTP_REQUEST_DEBUGGING) {
            qCDebug(networking) << "Received error response from data-server that has no matching callback.";
            qCDebug(networking) << "Error" << requestReply->error() << "-" << requestReply->errorString();
            qCDebug(networking) << requestReply->readAll();
        }
    }
}

void AccountManager::persistAccountToSettings() {
    if (_shouldPersistToSettingsFile) {
        // store this access token into the local settings
        QString keyURLString(_authURL.toString().replace("//", DOUBLE_SLASH_SUBSTITUTE));
        QStringList path = QStringList() << ACCOUNTS_GROUP << keyURLString;
        Setting::Handle<QVariant>(path).set(QVariant::fromValue(_accountInfo));
    }
}

bool AccountManager::hasValidAccessToken() {
    
    if (_accountInfo.getAccessToken().token.isEmpty() || _accountInfo.getAccessToken().isExpired()) {
        
        if (VERBOSE_HTTP_REQUEST_DEBUGGING) {
            qCDebug(networking) << "An access token is required for requests to" << qPrintable(_authURL.toString());
        }

        return false;
    } else {
        return true;
    }
}

bool AccountManager::checkAndSignalForAccessToken() {
    bool hasToken = hasValidAccessToken();

    if (!hasToken) {
        // emit a signal so somebody can call back to us and request an access token given a username and password
        emit authRequired();
    }

    return hasToken;
}

void AccountManager::setAccessTokenForCurrentAuthURL(const QString& accessToken) {
    // clear our current DataServerAccountInfo
    _accountInfo = DataServerAccountInfo();
    
    // start the new account info with a new OAuthAccessToken
    OAuthAccessToken newOAuthToken;
    newOAuthToken.token = accessToken;
    
    qCDebug(networking) << "Setting new account manager access token. F2C:" << accessToken.left(2) << "L2C:" << accessToken.right(2);
    
    _accountInfo.setAccessToken(newOAuthToken);
}

void AccountManager::requestAccessToken(const QString& login, const QString& password) {

    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);

    QUrl grantURL = _authURL;
    grantURL.setPath("/oauth/token");

    const QString ACCOUNT_MANAGER_REQUESTED_SCOPE = "owner";

    QByteArray postData;
    postData.append("grant_type=password&");
    postData.append("username=" + login + "&");
    postData.append("password=" + QUrl::toPercentEncoding(password) + "&");
    postData.append("scope=" + ACCOUNT_MANAGER_REQUESTED_SCOPE);

    request.setUrl(grantURL);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply* requestReply = networkAccessManager.post(request, postData);
    connect(requestReply, &QNetworkReply::finished, this, &AccountManager::requestAccessTokenFinished);
    connect(requestReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(requestAccessTokenError(QNetworkReply::NetworkError)));
}


void AccountManager::requestAccessTokenFinished() {
    QNetworkReply* requestReply = reinterpret_cast<QNetworkReply*>(sender());

    QJsonDocument jsonResponse = QJsonDocument::fromJson(requestReply->readAll());
    const QJsonObject& rootObject = jsonResponse.object();

    if (!rootObject.contains("error")) {
        // construct an OAuthAccessToken from the json object

        if (!rootObject.contains("access_token") || !rootObject.contains("expires_in")
            || !rootObject.contains("token_type")) {
            // TODO: error handling - malformed token response
            qCDebug(networking) << "Received a response for password grant that is missing one or more expected values.";
        } else {
            // clear the path from the response URL so we have the right root URL for this access token
            QUrl rootURL = requestReply->url();
            rootURL.setPath("");

            qCDebug(networking) << "Storing an account with access-token for" << qPrintable(rootURL.toString());

            _accountInfo = DataServerAccountInfo();
            _accountInfo.setAccessTokenFromJSON(rootObject);

            emit loginComplete(rootURL);
            
            persistAccountToSettings();

            requestProfile();
        }
    } else {
        // TODO: error handling
        qCDebug(networking) <<  "Error in response for password grant -" << rootObject["error_description"].toString();
        emit loginFailed();
    }
}

void AccountManager::requestAccessTokenError(QNetworkReply::NetworkError error) {
    // TODO: error handling
    qCDebug(networking) << "AccountManager requestError - " << error;
}

void AccountManager::requestProfile() {
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();

    QUrl profileURL = _authURL;
    profileURL.setPath("/api/v1/user/profile");
    
    QNetworkRequest profileRequest(profileURL);
    profileRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
    profileRequest.setRawHeader(ACCESS_TOKEN_AUTHORIZATION_HEADER, _accountInfo.getAccessToken().authorizationHeaderValue());

    QNetworkReply* profileReply = networkAccessManager.get(profileRequest);
    connect(profileReply, &QNetworkReply::finished, this, &AccountManager::requestProfileFinished);
    connect(profileReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(requestProfileError(QNetworkReply::NetworkError)));
}

void AccountManager::requestProfileFinished() {
    QNetworkReply* profileReply = reinterpret_cast<QNetworkReply*>(sender());

    QJsonDocument jsonResponse = QJsonDocument::fromJson(profileReply->readAll());
    const QJsonObject& rootObject = jsonResponse.object();

    if (rootObject.contains("status") && rootObject["status"].toString() == "success") {
        _accountInfo.setProfileInfoFromJSON(rootObject);

        emit profileChanged();

        // the username has changed to whatever came back
        emit usernameChanged(_accountInfo.getUsername());

        // store the whole profile into the local settings
        persistAccountToSettings();
        
    } else {
        // TODO: error handling
        qCDebug(networking) << "Error in response for profile";
    }
}

void AccountManager::requestProfileError(QNetworkReply::NetworkError error) {
    // TODO: error handling
    qCDebug(networking) << "AccountManager requestProfileError - " << error;
}

void AccountManager::generateNewKeypair() {
    // setup a new QThread to generate the keypair on, in case it takes a while
    QThread* generateThread = new QThread(this);
    generateThread->setObjectName("Account Manager Generator Thread");
    
    // setup a keypair generator
    RSAKeypairGenerator* keypairGenerator = new RSAKeypairGenerator();
    
    connect(generateThread, &QThread::started, keypairGenerator, &RSAKeypairGenerator::generateKeypair);
    connect(keypairGenerator, &RSAKeypairGenerator::generatedKeypair, this, &AccountManager::processGeneratedKeypair);
    connect(keypairGenerator, &RSAKeypairGenerator::errorGeneratingKeypair,
            this, &AccountManager::handleKeypairGenerationError);
    connect(keypairGenerator, &QObject::destroyed, generateThread, &QThread::quit);
    connect(generateThread, &QThread::finished, generateThread, &QThread::deleteLater);
    
    keypairGenerator->moveToThread(generateThread);
    
    qCDebug(networking) << "Starting worker thread to generate 2048-bit RSA key-pair.";
    generateThread->start();
}

void AccountManager::processGeneratedKeypair(const QByteArray& publicKey, const QByteArray& privateKey) {
    
    qCDebug(networking) << "Generated 2048-bit RSA key-pair. Storing private key and uploading public key.";
    
    // set the private key on our data-server account info
    _accountInfo.setPrivateKey(privateKey);
    persistAccountToSettings();
    
    // upload the public key so data-web has an up-to-date key
    const QString PUBLIC_KEY_UPDATE_PATH = "api/v1/user/public_key";
    
    // setup a multipart upload to send up the public key
    QHttpMultiPart* requestMultiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    QHttpPart keyPart;
    keyPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
    keyPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                      QVariant("form-data; name=\"public_key\"; filename=\"public_key\""));
    keyPart.setBody(publicKey);
    
    requestMultiPart->append(keyPart);
    
    sendRequest(PUBLIC_KEY_UPDATE_PATH, AccountManagerAuth::Required, QNetworkAccessManager::PutOperation,
                JSONCallbackParameters(), QByteArray(), requestMultiPart);
    
    // get rid of the keypair generator now that we don't need it anymore
    sender()->deleteLater();
}

void AccountManager::handleKeypairGenerationError() {
    // for now there isn't anything we do with this except get the worker thread to clean up
    sender()->deleteLater();
}
