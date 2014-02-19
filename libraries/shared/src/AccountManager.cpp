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
#include <QtCore/QUrlQuery>
#include <QtNetwork/QNetworkRequest>

#include "NodeList.h"
#include "PacketHeaders.h"

#include "AccountManager.h"

QMap<QUrl, OAuthAccessToken> AccountManager::_accessTokens = QMap<QUrl, OAuthAccessToken>();

AccountManager& AccountManager::getInstance() {
    static AccountManager sharedInstance;
    return sharedInstance;
}

Q_DECLARE_METATYPE(OAuthAccessToken)

const QString ACCOUNT_TOKEN_GROUP = "tokens";

AccountManager::AccountManager() :
    _rootURL(),
    _username(),
    _networkAccessManager(new QNetworkAccessManager)
{
    qRegisterMetaType<OAuthAccessToken>("OAuthAccessToken");
    qRegisterMetaTypeStreamOperators<OAuthAccessToken>("OAuthAccessToken");
    
    // check if there are existing access tokens to load from settings
    QSettings settings;
    settings.beginGroup(ACCOUNT_TOKEN_GROUP);
    
    foreach(const QString& key, settings.allKeys()) {
        // take a key copy to perform the double slash replacement
        QString keyCopy(key);
        QUrl keyURL(keyCopy.replace("slashslash", "//"));
        
        // pull out the stored access token and put it in our in memory array
        _accessTokens.insert(keyURL, settings.value(key).value<OAuthAccessToken>());
        qDebug() << "Found a data-server access token for" << qPrintable(keyURL.toString());
    }
}

void AccountManager::authenticatedGetRequest(const QString& path, const QObject *successReceiver, const char *successMethod,
                                             const QObject* errorReceiver, const char* errorMethod) {
    if (_networkAccessManager && hasValidAccessToken()) {
        QNetworkRequest authenticatedRequest;
        
        QUrl requestURL = _rootURL;
        requestURL.setPath(path);
        requestURL.setQuery("access_token=" + _accessTokens.value(_rootURL).token);
        
        authenticatedRequest.setUrl(requestURL);
        
        qDebug() << "Making an authenticated GET request to" << requestURL;
        
        QNetworkReply* networkReply = _networkAccessManager->get(authenticatedRequest);
        connect(networkReply, SIGNAL(finished()), successReceiver, successMethod);
        
        if (errorReceiver && errorMethod) {
            connect(networkReply, SIGNAL(error()), errorReceiver, errorMethod);
        }
    }
}

bool AccountManager::hasValidAccessToken() {
    OAuthAccessToken accessToken = _accessTokens.value(_rootURL);
    
    if (accessToken.token.isEmpty() || accessToken.isExpired()) {
        qDebug() << "An access token is required for requests to" << qPrintable(_rootURL.toString());
        return false;
    } else {
        return true;
    }
}

bool AccountManager::checkAndSignalForAccessToken() {
    bool hasToken = hasValidAccessToken();
    
    if (!hasToken) {
        // emit a signal so somebody can call back to us and request an access token given a username and password
        emit authenticationRequired();
    }
    
    return hasToken;
}

void AccountManager::requestAccessToken(const QString& login, const QString& password) {
    if (_networkAccessManager) {
        QNetworkRequest request;
        
        QUrl grantURL = _rootURL;
        grantURL.setPath("/oauth/token");
        
        QByteArray postData;
        postData.append("grant_type=password&");
        postData.append("username=" + login + "&");
        postData.append("password=" + password);
        
        request.setUrl(grantURL);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        
        QNetworkReply* requestReply = _networkAccessManager->post(request, postData);
        connect(requestReply, &QNetworkReply::finished, this, &AccountManager::requestFinished);
        connect(requestReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(requestError(QNetworkReply::NetworkError)));
    }
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
            
            qDebug() << "Storing an access token for" << rootURL;
            
            OAuthAccessToken freshAccessToken(rootObject);
            _accessTokens.insert(rootURL, freshAccessToken);
            
            // store this access token into the local settings
            QSettings localSettings;
            localSettings.beginGroup(ACCOUNT_TOKEN_GROUP);
            localSettings.setValue(rootURL.toString().replace("//", "slashslash"), QVariant::fromValue(freshAccessToken));
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