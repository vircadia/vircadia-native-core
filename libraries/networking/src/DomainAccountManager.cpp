//
//  DomainAccountManager.cpp
//  libraries/networking/src
//
//  Created by David Rowe on 23 Jul 2020.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DomainAccountManager.h"

#include <SettingHandle.h>

#include <QTimer>
#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QHttpMultiPart>

#include "DomainAccountManager.h"
#include "NetworkingConstants.h"
#include "OAuthAccessToken.h"
#include "NetworkLogging.h"
#include "NodeList.h"
#include "udt/PacketHeaders.h"
#include "NetworkAccessManager.h"

// FIXME: Generalize to other OAuth2 sources for domain login.

const bool VERBOSE_HTTP_REQUEST_DEBUGGING = false;
const QString DOMAIN_ACCOUNT_MANAGER_REQUESTED_SCOPE = "foo bar";  // ####### TODO: WordPress plugin's required scope.
// ####### TODO: Should scope be configured in domain server settings?

// ####### TODO: Add storing domain URL and check against it when retrieving values?
// ####### TODO: Add storing _authURL and check against it when retrieving values?
Setting::Handle<QString> domainAccessToken {"private/domainAccessToken", "" };
Setting::Handle<QString> domainAccessRefreshToken {"private/domainAccessToken", "" };
Setting::Handle<int> domainAccessTokenExpiresIn {"private/domainAccessTokenExpiresIn", -1 };
Setting::Handle<QString> domainAccessTokenType {"private/domainAccessTokenType", "" };


DomainAccountManager::DomainAccountManager() {
    connect(this, &DomainAccountManager::loginComplete, this, &DomainAccountManager::sendInterfaceAccessTokenToServer);
}

void DomainAccountManager::setAuthURL(const QUrl& authURL) {
    if (_authURL != authURL) {
        _authURL = authURL;

        qCDebug(networking) << "AccountManager URL for authenticated requests has been changed to" << qPrintable(_authURL.toString());

        // ####### TODO: See AccountManager::setAuthURL().
    }
}

void DomainAccountManager::requestAccessToken(const QString& login, const QString& password) {


    QNetworkRequest request;

    request.setHeader(QNetworkRequest::UserAgentHeader, NetworkingConstants::VIRCADIA_USER_AGENT);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    // ####### TODO: WordPress plugin's authorization requirements.
    request.setRawHeader(QByteArray("Authorization"), QByteArray("Basic b2F1dGgtY2xpZW50LTE6b2F1dGgtY2xpZW50LXNlY3JldC0x"));

    QByteArray formData;
    formData.append("grant_type=password&");
    formData.append("username=" + QUrl::toPercentEncoding(login) + "&");
    formData.append("password=" + QUrl::toPercentEncoding(password) + "&");
    formData.append("scope=" + DOMAIN_ACCOUNT_MANAGER_REQUESTED_SCOPE);
    // ####### TODO: Include state?

    QUrl domainAuthURL = _authURL;
    domainAuthURL.setPath("/token");  // ####### TODO: miniOrange-mandated URL. ####### TODO: Should this be included in the server settings value?
    request.setUrl(domainAuthURL);

    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkReply* requestReply = networkAccessManager.post(request, formData);
    connect(requestReply, &QNetworkReply::finished, this, &DomainAccountManager::requestAccessTokenFinished);
}

void DomainAccountManager::requestAccessTokenFinished() {
    QNetworkReply* requestReply = reinterpret_cast<QNetworkReply*>(sender());

    QJsonDocument jsonResponse = QJsonDocument::fromJson(requestReply->readAll());
    const QJsonObject& rootObject = jsonResponse.object();

    if (!rootObject.contains("error")) {
        // ####### TODO: Process response scope?
        // ####### TODO: Process response state?

        if (!rootObject.contains("access_token") 
            // ####### TODO: Does WordPRess plugin provide "expires_in"?
            //               If so, handle here, or is it just the domain server that needs to use it?
            //|| !rootObject.contains("expires_in")  
            || !rootObject.contains("token_type")) {

            qCDebug(networking) << "Received a response for password grant that is missing one or more expected values.";
            emit loginFailed();

        } else {

            // clear the path from the response URL so we have the right root URL for this access token
            QUrl rootURL = requestReply->url();
            rootURL.setPath("");

            qCDebug(networking) << "Storing a domain account with access-token for" << qPrintable(rootURL.toString());
            setAccessTokenFromJSON(rootObject);

            emit loginComplete(rootURL);
        }
    } else {
        qCDebug(networking) <<  "Error in response for password grant -" << rootObject["error_description"].toString();
        emit loginFailed();
    }
}

void DomainAccountManager::sendInterfaceAccessTokenToServer() {
    // TODO: Send successful packet to the domain-server.
}

bool DomainAccountManager::accessTokenIsExpired() {
    // ####### TODO: accessTokenIsExpired()
    return true;
    /*
    return domainAccessTokenExpiresIn.get() != -1 && domainAccessTokenExpiresIn.get() <= QDateTime::currentMSecsSinceEpoch(); 
    */
}


bool DomainAccountManager::hasValidAccessToken() {
    QString currentDomainAccessToken = domainAccessToken.get();
    
    if (currentDomainAccessToken.isEmpty() || accessTokenIsExpired()) {

        if (VERBOSE_HTTP_REQUEST_DEBUGGING) {
            qCDebug(networking) << "An access token is required for requests to"
                                << qPrintable(_authURL.toString());
        }

        return false;
    } else {

        // ####### TODO::
        
        // if (!_isWaitingForTokenRefresh && needsToRefreshToken()) {
        //     refreshAccessToken();
        // }

        return true;
    }
    
}

void DomainAccountManager::setAccessTokenFromJSON(const QJsonObject& jsonObject) {
    // ####### TODO: Enable and use these.
    /*
    domainAccessToken.set(jsonObject["access_token"].toString());
    domainAccessRefreshToken.set(jsonObject["refresh_token"].toString());
    domainAccessTokenExpiresIn.set(QDateTime::currentMSecsSinceEpoch() + (jsonObject["expires_in"].toDouble() * 1000));
    domainAccessTokenType.set(jsonObject["token_type"].toString());
    */
}

bool DomainAccountManager::checkAndSignalForAccessToken() {
    bool hasToken = hasValidAccessToken();

    // ####### TODO: Handle hasToken == true. 
    // It causes the login dialog not to display (OK) but somewhere the domain server needs to be sent it (and if domain server 
    // gets error when trying to use it then user should be prompted to login).
    hasToken = false;
    
    if (!hasToken) {
        // Emit a signal so somebody can call back to us and request an access token given a user name and password.

        // Dialog can be hidden immediately after showing if we've just teleported to the domain, unless the signal is delayed.
        QTimer::singleShot(500, this, [this] { emit this->authRequired(); });
    }

    return hasToken;
}
