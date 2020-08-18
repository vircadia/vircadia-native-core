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

#include <QTimer>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <DependencyManager.h>
#include <SettingHandle.h>

#include "NetworkingConstants.h"
#include "NetworkAccessManager.h"
#include "NetworkLogging.h"
#include "NodeList.h"

// FIXME: Generalize to other OAuth2 sources for domain login.

const bool VERBOSE_HTTP_REQUEST_DEBUGGING = false;

// ####### TODO: Enable and use these?
// ####### TODO: Add storing domain URL and check against it when retrieving values?
// ####### TODO: Add storing _authURL and check against it when retrieving values?
/*
Setting::Handle<QString> domainAccessToken {"private/domainAccessToken", "" };
Setting::Handle<QString> domainAccessRefreshToken {"private/domainAccessToken", "" };
Setting::Handle<int> domainAccessTokenExpiresIn {"private/domainAccessTokenExpiresIn", -1 };
Setting::Handle<QString> domainAccessTokenType {"private/domainAccessTokenType", "" };
*/

DomainAccountManager::DomainAccountManager() {
    connect(this, &DomainAccountManager::loginComplete, this, &DomainAccountManager::sendInterfaceAccessTokenToServer);
}

void DomainAccountManager::setDomainURL(const QUrl& domainURL) {

    if (domainURL == _domainURL) {
        return;
    }

    _domainURL = domainURL;
    qCDebug(networking) << "DomainAccountManager domain URL has been changed to" << qPrintable(_domainURL.toString());

    // Restore OAuth2 authorization if have it for this domain.
    if (_domainURL == _previousDomainURL) {
        _authURL = _previousAuthURL;
        _clientID = _previousClientID;
        _username = _previousUsername;
        _accessToken = _previousAccessToken;
        _refreshToken = _previousRefreshToken;
        _authedDomainName = _previousAuthedDomainName;

        // ####### TODO: Refresh OAuth2 access token if necessary.

    } else {
        _authURL.clear();
        _clientID.clear();
        _username.clear();
        _accessToken.clear();
        _refreshToken.clear();
        _authedDomainName.clear();
    }

}

void DomainAccountManager::setAuthURL(const QUrl& authURL) {
    if (_authURL == authURL) {
        return;
    }

    _authURL = authURL;
    qCDebug(networking) << "DomainAccountManager URL for authenticated requests has been changed to" 
        << qPrintable(_authURL.toString());

    _accessToken = "";
    _refreshToken = "";
}

bool DomainAccountManager::hasLogIn() {
    return !_authURL.isEmpty();
}

bool DomainAccountManager::isLoggedIn() { 
    return !_authURL.isEmpty() && hasValidAccessToken();
}

void DomainAccountManager::requestAccessToken(const QString& username, const QString& password) {

    _username = username;
    _accessToken = "";
    _refreshToken = "";

    QNetworkRequest request;

    request.setHeader(QNetworkRequest::UserAgentHeader, NetworkingConstants::VIRCADIA_USER_AGENT);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // miniOrange WordPress API Authentication plugin:
    // - Requires "client_id" parameter.
    // - Ignores "state" parameter.
    QByteArray formData;
    formData.append("grant_type=password&");
    formData.append("username=" + QUrl::toPercentEncoding(username) + "&");
    formData.append("password=" + QUrl::toPercentEncoding(password) + "&");
    formData.append("client_id=" + _clientID);

    request.setUrl(_authURL);

    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkReply* requestReply = networkAccessManager.post(request, formData);
    connect(requestReply, &QNetworkReply::finished, this, &DomainAccountManager::requestAccessTokenFinished);
}

void DomainAccountManager::requestAccessTokenFinished() {

    QNetworkReply* requestReply = reinterpret_cast<QNetworkReply*>(sender());

    QJsonDocument jsonResponse = QJsonDocument::fromJson(requestReply->readAll());
    const QJsonObject& rootObject = jsonResponse.object();

    auto httpStatus = requestReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (200 <= httpStatus && httpStatus < 300) {

        // miniOrange plugin provides no scope.
        if (rootObject.contains("access_token")) {
            // Success.
            auto nodeList = DependencyManager::get<NodeList>();
            _authedDomainName = nodeList->getDomainHandler().getHostname();
            QUrl rootURL = requestReply->url();
            rootURL.setPath("");
            setTokensFromJSON(rootObject, rootURL);

            // Remember domain login for the current Interface session.
            _previousDomainURL = _domainURL;
            _previousAuthURL = _authURL;
            _previousClientID = _clientID;
            _previousUsername = _username;
            _previousAccessToken = _accessToken;
            _previousRefreshToken = _refreshToken;
            _previousAuthedDomainName = _authedDomainName;

            // ####### TODO: Handle "keep me logged in".

            emit loginComplete();
        } else {
            // Failure.
            qCDebug(networking) << "Received a response for password grant that is missing one or more expected values.";
            emit loginFailed();
        }

    } else {
        // Failure.
        qCDebug(networking) << "Error in response for password grant -" << httpStatus << requestReply->error()
            << "-" << rootObject["error"].toString() << rootObject["error_description"].toString();
        emit loginFailed();
    }
}

void DomainAccountManager::sendInterfaceAccessTokenToServer() {
    emit newTokens();
}

bool DomainAccountManager::accessTokenIsExpired() {
    // ####### TODO: accessTokenIsExpired()
    return true;
    /*
    return domainAccessTokenExpiresIn.get() != -1 && domainAccessTokenExpiresIn.get() <= QDateTime::currentMSecsSinceEpoch(); 
    */
}


bool DomainAccountManager::hasValidAccessToken() {
    // ###### TODO: wire this up to actually retrieve a token (based on session or storage) and confirm that it is in fact valid and relevant to the current domain.
    // QString currentDomainAccessToken = domainAccessToken.get();
    QString currentDomainAccessToken = _accessToken;

    // if (currentDomainAccessToken.isEmpty() || accessTokenIsExpired()) {
    if (currentDomainAccessToken.isEmpty()) {
        if (VERBOSE_HTTP_REQUEST_DEBUGGING) {
            qCDebug(networking) << "An access token is required for requests to"
                                << qPrintable(_authURL.toString());
        }

        return false;
    }

    // ####### TODO
        
    // if (!_isWaitingForTokenRefresh && needsToRefreshToken()) {
    //     refreshAccessToken();
    // }

    return true;
}

void DomainAccountManager::setTokensFromJSON(const QJsonObject& jsonObject, const QUrl& url) {
    _accessToken = jsonObject["access_token"].toString();
    _refreshToken = jsonObject["refresh_token"].toString();

    // ####### TODO: Enable and use these?
    // ####### TODO: Protect these per AccountManager?
    // ######: TODO: clientID needed?
    // qCDebug(networking) << "Storing a domain account with access-token for" << qPrintable(url.toString());
    // domainAccessToken.set(jsonObject["access_token"].toString());
    // domainAccessRefreshToken.set(jsonObject["refresh_token"].toString());
    // domainAccessTokenExpiresIn.set(QDateTime::currentMSecsSinceEpoch() + (jsonObject["expires_in"].toDouble() * 1000));
    // domainAccessTokenType.set(jsonObject["token_type"].toString());
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
        auto domain = _authURL.host();
        QTimer::singleShot(500, this, [this, domain] {
            emit this->authRequired(domain); 
        });
    }

    return hasToken;
}
