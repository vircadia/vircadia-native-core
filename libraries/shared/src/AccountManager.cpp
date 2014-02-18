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

AccountManager& AccountManager::getInstance() {
    static AccountManager sharedInstance;
    return sharedInstance;
}

const QString DEFAULT_NODE_AUTH_OAUTH_CLIENT_ID = "12b7b18e7b8c118707b84ff0735e57a4473b5b0577c2af44734f02e08d02829c";

AccountManager::AccountManager() :
    _username(),
    _accessTokens(),
    _clientIDs(),
    _networkAccessManager(NULL)
{
    
    _clientIDs.insert(DEFAULT_NODE_AUTH_URL, DEFAULT_NODE_AUTH_OAUTH_CLIENT_ID);
}

bool AccountManager::hasValidAccessTokenForRootURL(const QUrl &rootURL) {
    OAuthAccessToken accessToken = _accessTokens.value(rootURL);
    
    if (accessToken.token.isEmpty() || accessToken.isExpired()) {
        // emit a signal so somebody can call back to us and request an access token given a username and password
        qDebug() << "An access token is required for requests to" << qPrintable(rootURL.toString());
        emit authenticationRequiredForRootURL(rootURL);
        return false;
    } else {
        return true;
    }
}

void AccountManager::requestAccessToken(const QUrl& rootURL, const QString& username, const QString& password) {
    if (_networkAccessManager) {
        if (_clientIDs.contains(rootURL)) {
            QNetworkRequest request;
            
            QUrl grantURL = rootURL;
            grantURL.setPath("/oauth/token");
            
            QByteArray postData;
            postData.append("client_id=" + _clientIDs.value(rootURL) + "&");
            postData.append("grant_type=password&");
            postData.append("username=" + username + "&");
            postData.append("password=" + password);
            
            request.setUrl(grantURL);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
            
            QNetworkReply* requestReply = _networkAccessManager->post(request, postData);
            connect(requestReply, &QNetworkReply::finished, this, &AccountManager::requestFinished);
            connect(requestReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(requestError(QNetworkReply::NetworkError)));
        } else {
            qDebug() << "Client ID for OAuth authorization at" << rootURL.toString() << "is unknown. Cannot authenticate.";
        }
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
            
            _accessTokens.insert(requestReply->url(), OAuthAccessToken(rootObject));
        }
    } else {
        // TODO: error handling
        qDebug() <<  "Error in response for password grant -" << rootObject["error_description"].toString();
    }
}

void AccountManager::requestError(QNetworkReply::NetworkError error) {
    
}