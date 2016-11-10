//
//  HFWebEngineRequestInterceptor.cpp
//  interface/src/networking
//
//  Created by Stephen Birarda on 2016-10-14.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HFWebEngineRequestInterceptor.h"

#include <QtCore/QDebug>

#include <AccountManager.h>

bool isAuthableHighFidelityURL(const QUrl& url) {
    static const QStringList HF_HOSTS = {
        "highfidelity.com", "highfidelity.io",
        "metaverse.highfidelity.com", "metaverse.highfidelity.io"
    };

    return url.scheme() == "https" && HF_HOSTS.contains(url.host());
}

void HFWebEngineRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo& info) {
    // check if this is a request to a highfidelity URL
    if (isAuthableHighFidelityURL(info.requestUrl())) {
        // if we have an access token, add it to the right HTTP header for authorization
        auto accountManager = DependencyManager::get<AccountManager>();

        if (accountManager->hasValidAccessToken()) {
            static const QString OAUTH_AUTHORIZATION_HEADER = "Authorization";

            QString bearerTokenString = "Bearer " + accountManager->getAccountInfo().getAccessToken().token;
            info.setHttpHeader(OAUTH_AUTHORIZATION_HEADER.toLocal8Bit(), bearerTokenString.toLocal8Bit());
        }
    }
}
