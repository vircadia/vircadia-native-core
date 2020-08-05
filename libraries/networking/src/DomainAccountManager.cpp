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

#include <SettingHandle.h>
#include <DependencyManager.h>

#include "NodeList.h"
#include "NetworkingConstants.h"
#include "NetworkLogging.h"
#include "NetworkAccessManager.h"

// FIXME: Generalize to other OAuth2 sources for domain login.

const bool VERBOSE_HTTP_REQUEST_DEBUGGING = false;

// ####### TODO: Add storing domain URL and check against it when retrieving values?
// ####### TODO: Add storing _authURL and check against it when retrieving values?
Setting::Handle<QString> domainAccessToken {"private/domainAccessToken", "" };
Setting::Handle<QString> domainAccessRefreshToken {"private/domainAccessToken", "" };
Setting::Handle<int> domainAccessTokenExpiresIn {"private/domainAccessTokenExpiresIn", -1 };
Setting::Handle<QString> domainAccessTokenType {"private/domainAccessTokenType", "" };


DomainAccountManager::DomainAccountManager() : 
    _authURL(),
    _username(),
    _access_token(),
    _refresh_token()
{
    connect(this, &DomainAccountManager::loginComplete, this, &DomainAccountManager::sendInterfaceAccessTokenToServer);
}

void DomainAccountManager::setAuthURL(const QUrl& authURL) {
    if (_authURL != authURL) {
        _authURL = authURL;

        qCDebug(networking) << "AccountManager URL for authenticated requests has been changed to" << qPrintable(_authURL.toString());

        _access_token = "";
        _refresh_token = "";

        // ####### TODO: Restore and refresh OAuth2 tokens if have them for this domain.

        // ####### TODO: Handle "keep me logged in".
    }
}

void DomainAccountManager::requestAccessToken(const QString& username, const QString& password) {

    _username = username;
    _access_token = "";
    _refresh_token = "";

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
            _domain_name = nodeList->getDomainHandler().getHostname();
            QUrl rootURL = requestReply->url();
            rootURL.setPath("");
            setTokensFromJSON(rootObject, rootURL);
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
    QString currentDomainAccessToken = _access_token;

    // if (currentDomainAccessToken.isEmpty() || accessTokenIsExpired()) {
    if (currentDomainAccessToken.isEmpty()) {
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

void DomainAccountManager::setTokensFromJSON(const QJsonObject& jsonObject, const QUrl& url) {
    _access_token = jsonObject["access_token"].toString();
    _refresh_token = jsonObject["refresh_token"].toString();

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
