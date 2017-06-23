//
//  HFTabletWebEngineRequestInterceptor.cpp
//  interface/src/networking
//
//  Created by Dante Ruiz on 2017-3-31.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HFTabletWebEngineRequestInterceptor.h"
#include <QtCore/QDebug>
#include "AccountManager.h"

bool isTabletAuthableHighFidelityURL(const QUrl& url) {
    static const QStringList HF_HOSTS = {
        "highfidelity.com", "highfidelity.io",
        "metaverse.highfidelity.com", "metaverse.highfidelity.io"
    };

    return url.scheme() == "https" && HF_HOSTS.contains(url.host());
}

void HFTabletWebEngineRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo& info) {
    // check if this is a request to a highfidelity URL
    if (isTabletAuthableHighFidelityURL(info.requestUrl())) {
        // if we have an access token, add it to the right HTTP header for authorization
        auto accountManager = DependencyManager::get<AccountManager>();

        if (accountManager->hasValidAccessToken()) {
            static const QString OAUTH_AUTHORIZATION_HEADER = "Authorization";

            QString bearerTokenString = "Bearer " + accountManager->getAccountInfo().getAccessToken().token;
            info.setHttpHeader(OAUTH_AUTHORIZATION_HEADER.toLocal8Bit(), bearerTokenString.toLocal8Bit());
        }

        static const QString USER_AGENT = "User-Agent";
        QString tokenString = "Chrome/48.0 (HighFidelityInterface)";
        info.setHttpHeader(USER_AGENT.toLocal8Bit(), tokenString.toLocal8Bit());
    } else {
        static const QString USER_AGENT = "User-Agent";
        QString tokenString = "Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2924.87 Mobile Safari/537.36";
        info.setHttpHeader(USER_AGENT.toLocal8Bit(), tokenString.toLocal8Bit());
    }
}
