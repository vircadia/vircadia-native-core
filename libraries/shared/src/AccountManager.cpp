//
//  AccountManager.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QDataStream>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMap>
#include <QtCore/QStringList>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QNetworkRequest>

#include "NodeList.h"
#include "PacketHeaders.h"

#include "AccountManager.h"

const bool VERBOSE_HTTP_REQUEST_DEBUGGING = false;

AccountManager& AccountManager::getInstance() {
    static AccountManager sharedInstance;
    return sharedInstance;
}

Q_DECLARE_METATYPE(OAuthAccessToken)
Q_DECLARE_METATYPE(DataServerAccountInfo)
Q_DECLARE_METATYPE(QNetworkAccessManager::Operation)
Q_DECLARE_METATYPE(JSONCallbackParameters)

const QString ACCOUNTS_GROUP = "accounts";

AccountManager::AccountManager() :
    _rootURL(),
    _networkAccessManager(),
    _pendingCallbackMap(),
    _accounts()
{
    qRegisterMetaType<OAuthAccessToken>("OAuthAccessToken");
    qRegisterMetaTypeStreamOperators<OAuthAccessToken>("OAuthAccessToken");
    
    qRegisterMetaType<DataServerAccountInfo>("DataServerAccountInfo");
    qRegisterMetaTypeStreamOperators<DataServerAccountInfo>("DataServerAccountInfo");
    
    qRegisterMetaType<QNetworkAccessManager::Operation>("QNetworkAccessManager::Operation");
    qRegisterMetaType<JSONCallbackParameters>("JSONCallbackParameters");
    
    // check if there are existing access tokens to load from settings
    QSettings settings;
    settings.beginGroup(ACCOUNTS_GROUP);
    
    foreach(const QString& key, settings.allKeys()) {
        // take a key copy to perform the double slash replacement
        QString keyCopy(key);
        QUrl keyURL(keyCopy.replace("slashslash", "//"));
        
        // pull out the stored access token and put it in our in memory array
        _accounts.insert(keyURL, settings.value(key).value<DataServerAccountInfo>());
        qDebug() << "Found a data-server access token for" << qPrintable(keyURL.toString());
    }
}

const QString DOUBLE_SLASH_SUBSTITUTE = "slashslash";

void AccountManager::logout() {
    // a logout means we want to delete the DataServerAccountInfo we currently have for this URL, in-memory and in file
    _accounts.remove(_rootURL);
    
    QSettings settings;
    settings.beginGroup(ACCOUNTS_GROUP);
    
    QString keyURLString(_rootURL.toString().replace("//", DOUBLE_SLASH_SUBSTITUTE));
    settings.remove(keyURLString);
    
    qDebug() << "Removed account info for" << _rootURL << "from in-memory accounts and .ini file";
    
    emit logoutComplete();
    // the username has changed to blank
    emit usernameChanged(QString());
    
}

void AccountManager::setRootURL(const QUrl& rootURL) {
    if (_rootURL != rootURL) {
        _rootURL = rootURL;
        
        qDebug() << "URL for node authentication has been changed to" << qPrintable(_rootURL.toString());
        qDebug() << "Re-setting authentication flow.";
        
        // tell listeners that the auth endpoint has changed
        emit authEndpointChanged();
    }
}

void AccountManager::authenticatedRequest(const QString& path, QNetworkAccessManager::Operation operation,
                                          const JSONCallbackParameters& callbackParams, const QByteArray& dataByteArray) {
    QMetaObject::invokeMethod(this, "invokedRequest",
                              Q_ARG(const QString&, path),
                              Q_ARG(QNetworkAccessManager::Operation, operation),
                              Q_ARG(const JSONCallbackParameters&, callbackParams),
                              Q_ARG(const QByteArray&, dataByteArray));
}

void AccountManager::invokedRequest(const QString& path, QNetworkAccessManager::Operation operation,
                                    const JSONCallbackParameters& callbackParams, const QByteArray& dataByteArray) {
    if (hasValidAccessToken()) {
        QNetworkRequest authenticatedRequest;
        
        QUrl requestURL = _rootURL;
        requestURL.setPath(path);
        requestURL.setQuery("access_token=" + _accounts.value(_rootURL).getAccessToken().token);
        
        authenticatedRequest.setUrl(requestURL);
        
        if (VERBOSE_HTTP_REQUEST_DEBUGGING) {
            qDebug() << "Making an authenticated request to" << qPrintable(requestURL.toString());
            
            if (!dataByteArray.isEmpty()) {
                qDebug() << "The POST/PUT body -" << QString(dataByteArray);
            }
        }
        
        QNetworkReply* networkReply = NULL;
        
        switch (operation) {
            case QNetworkAccessManager::GetOperation:
                networkReply = _networkAccessManager.get(authenticatedRequest);
                break;
            case QNetworkAccessManager::PostOperation:
            case QNetworkAccessManager::PutOperation:
                authenticatedRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
                
                if (operation == QNetworkAccessManager::PostOperation) {
                    networkReply = _networkAccessManager.post(authenticatedRequest, dataByteArray);
                } else {
                    networkReply = _networkAccessManager.put(authenticatedRequest, dataByteArray);
                }
                
                break;
            default:
                // other methods not yet handled
                break;
        }
        
        if (networkReply) {
            if (!callbackParams.isEmpty()) {
                // if we have information for a callback, insert the callbackParams into our local map
                _pendingCallbackMap.insert(networkReply, callbackParams);
            }
            
            // if we ended up firing of a request, hook up to it now
            connect(networkReply, SIGNAL(finished()), this, SLOT(passSuccessToCallback()));
            connect(networkReply, SIGNAL(error(QNetworkReply::NetworkError)),
                    this, SLOT(passErrorToCallback(QNetworkReply::NetworkError)));
        }
    }
}

void AccountManager::passSuccessToCallback() {
    QNetworkReply* requestReply = reinterpret_cast<QNetworkReply*>(sender());
    QJsonDocument jsonResponse = QJsonDocument::fromJson(requestReply->readAll());
    
    JSONCallbackParameters callbackParams = _pendingCallbackMap.value(requestReply);
    
    if (callbackParams.jsonCallbackReceiver) {
        // invoke the right method on the callback receiver
        QMetaObject::invokeMethod(callbackParams.jsonCallbackReceiver, qPrintable(callbackParams.jsonCallbackMethod),
                                  Q_ARG(const QJsonObject&, jsonResponse.object()));
        
        // remove the related reply-callback group from the map
        _pendingCallbackMap.remove(requestReply);
        
    } else {
        if (VERBOSE_HTTP_REQUEST_DEBUGGING) {
            qDebug() << "Received JSON response from data-server that has no matching callback.";
            qDebug() << jsonResponse;
        }
    }
}

void AccountManager::passErrorToCallback(QNetworkReply::NetworkError errorCode) {
    QNetworkReply* requestReply = reinterpret_cast<QNetworkReply*>(sender());
    JSONCallbackParameters callbackParams = _pendingCallbackMap.value(requestReply);
    
    if (callbackParams.errorCallbackReceiver) {
        // invoke the right method on the callback receiver
        QMetaObject::invokeMethod(callbackParams.errorCallbackReceiver, qPrintable(callbackParams.errorCallbackMethod),
                                  Q_ARG(QNetworkReply::NetworkError, errorCode),
                                  Q_ARG(const QString&, requestReply->errorString()));
        
        // remove the related reply-callback group from the map
        _pendingCallbackMap.remove(requestReply);
    } else {
        if (VERBOSE_HTTP_REQUEST_DEBUGGING) {
            qDebug() << "Received error response from data-server that has no matching callback.";
            qDebug() << "Error" << errorCode << "-" << requestReply->errorString();
        }
    }
}

bool AccountManager::hasValidAccessToken() {
    DataServerAccountInfo accountInfo = _accounts.value(_rootURL);
    
    if (accountInfo.getAccessToken().token.isEmpty() || accountInfo.getAccessToken().isExpired()) {
        if (VERBOSE_HTTP_REQUEST_DEBUGGING) {
            qDebug() << "An access token is required for requests to" << qPrintable(_rootURL.toString());
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

void AccountManager::requestAccessToken(const QString& login, const QString& password) {
    QNetworkRequest request;
    
    QUrl grantURL = _rootURL;
    grantURL.setPath("/oauth/token");
    
    QByteArray postData;
    postData.append("grant_type=password&");
    postData.append("username=" + login + "&");
    postData.append("password=" + password);
    
    request.setUrl(grantURL);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    
    QNetworkReply* requestReply = _networkAccessManager.post(request, postData);
    connect(requestReply, &QNetworkReply::finished, this, &AccountManager::requestFinished);
    connect(requestReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(requestError(QNetworkReply::NetworkError)));
}


void AccountManager::requestFinished() {
    QNetworkReply* requestReply = reinterpret_cast<QNetworkReply*>(sender());
    
    QJsonDocument jsonResponse = QJsonDocument::fromJson(requestReply->readAll());
    const QJsonObject& rootObject = jsonResponse.object();
    
    if (!rootObject.contains("error")) {
        // construct an OAuthAccessToken from the json object
        
        if (!rootObject.contains("access_token") || !rootObject.contains("expires_in")
            || !rootObject.contains("token_type")) {
            // TODO: error handling - malformed token response
            qDebug() << "Received a response for password grant that is missing one or more expected values.";
        } else {
            // clear the path from the response URL so we have the right root URL for this access token
            QUrl rootURL = requestReply->url();
            rootURL.setPath("");
            
            qDebug() << "Storing an account with access-token for" << qPrintable(rootURL.toString());
            
            DataServerAccountInfo freshAccountInfo(rootObject);
            _accounts.insert(rootURL, freshAccountInfo);
            
            emit loginComplete(rootURL);
            // the username has changed to whatever came back
            emit usernameChanged(freshAccountInfo.getUsername());
            
            // store this access token into the local settings
            QSettings localSettings;
            localSettings.beginGroup(ACCOUNTS_GROUP);
            localSettings.setValue(rootURL.toString().replace("//", DOUBLE_SLASH_SUBSTITUTE),
                                   QVariant::fromValue(freshAccountInfo));
        }
    } else {
        // TODO: error handling
        qDebug() <<  "Error in response for password grant -" << rootObject["error_description"].toString();
    }
}

void AccountManager::requestError(QNetworkReply::NetworkError error) {
    // TODO: error handling
    qDebug() << "AccountManager requestError - " << error;
}