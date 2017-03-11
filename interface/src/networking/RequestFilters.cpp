//
//  RequestFilters.cpp
//  interface/src/networking
//
//  Created by Kunal Gosar on 2017-03-10.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RequestFilters.h"

#include <QtCore/QDebug>

#include <AccountManager.h>

bool isAuthableHighFidelityURL(const QUrl& url) {
    static const QStringList HF_HOSTS = {
        "highfidelity.com", "highfidelity.io",
        "metaverse.highfidelity.com", "metaverse.highfidelity.io"
    };

    return url.scheme() == "https" && HF_HOSTS.contains(url.host());
}

bool isJavaScriptFile(const QString filename) {
    return filename.contains(".js", Qt::CaseInsensitive);
}

bool isEntityFile(const QString filename) {
    return filename.contains(".svo.json", Qt::CaseInsensitive);
}

void RequestFilters::interceptHFWebEngineRequest(QWebEngineUrlRequestInfo& info) {
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

void RequestFilters::interceptFileType(QWebEngineUrlRequestInfo& info) {
    QString filename = info.requestUrl().fileName();
    
    if (isJavaScriptFile(filename) || isEntityFile(filename)) {
        static const QString CONTENT_HEADER = "Accept";
        static const QString TYPE_VALUE = "text/html";
        info.setHttpHeader(CONTENT_HEADER.toLocal8Bit(), TYPE_VALUE.toLocal8Bit());
        static const QString CONTENT_DISPOSITION = "Content-Disposition";
        static const QString DISPOSITION_VALUE = "inline";
        info.setHttpHeader(CONTENT_DISPOSITION.toLocal8Bit(), DISPOSITION_VALUE.toLocal8Bit());
    }
}