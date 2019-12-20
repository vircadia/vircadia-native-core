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
#include <QtCore/QFileInfo>

#include <SettingHandle.h>
#include <NetworkingConstants.h>
#include <MetaverseAPI.h>
#include <AccountManager.h>

#include "ContextAwareProfile.h"

#if !defined(Q_OS_ANDROID)

namespace {

    bool isAuthableHighFidelityURL(const QUrl& url) {
        auto metaverseServerURL = MetaverseAPI::getCurrentMetaverseServerURL();
        static const QStringList HF_HOSTS = {
            "highfidelity.com", "highfidelity.io",
            metaverseServerURL.toString(), "metaverse.highfidelity.io"
        };
        const auto& scheme = url.scheme();
        const auto& host = url.host();

        return (scheme == "https" && HF_HOSTS.contains(host)) ||
            ((scheme == metaverseServerURL.scheme()) && (host == metaverseServerURL.host()));
    }

     bool isScript(const QString filename) {
         return filename.endsWith(".js", Qt::CaseInsensitive);
     }

     bool isJSON(const QString filename) {
        return filename.endsWith(".json", Qt::CaseInsensitive);
     }

     bool blockLocalFiles(QWebEngineUrlRequestInfo& info) {
         auto requestUrl = info.requestUrl();
         if (!requestUrl.isLocalFile()) {
             // Not a local file, do not block
             return false;
         }

         // We can potentially add whitelisting logic or development environment variables that
         // will allow people to override this setting on a per-client basis here.
         QString targetFilePath = QFileInfo(requestUrl.toLocalFile()).canonicalFilePath();

         // If we get here, we've determined it's a local file and we have no reason not to block it
         qWarning() << "Blocking web access to local file path" << targetFilePath;
         info.block(true);
         return true;
     }
}

void RequestFilters::interceptHFWebEngineRequest(QWebEngineUrlRequestInfo& info, bool restricted) {
    if (restricted && blockLocalFiles(info)) {
        return;
    }

    // check if this is a request to a highfidelity URL
    bool isAuthable = isAuthableHighFidelityURL(info.requestUrl());
    auto accountManager = DependencyManager::get<AccountManager>();
    if (isAuthable) {
        // if we have an access token, add it to the right HTTP header for authorization

        if (accountManager->hasValidAccessToken()) {
            static const QString OAUTH_AUTHORIZATION_HEADER = "Authorization";

            QString bearerTokenString = "Bearer " + accountManager->getAccountInfo().getAccessToken().token;
            info.setHttpHeader(OAUTH_AUTHORIZATION_HEADER.toLocal8Bit(), bearerTokenString.toLocal8Bit());
        }
    }
    static const QString USER_AGENT = "User-Agent";
    const QString tokenStringMobile{ "Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2924.87 Mobile Safari/537.36" };
    const QString tokenStringMetaverse{ "Chrome/48.0 (HighFidelityInterface)" };
    const QString tokenStringLimitedCommerce{ "Chrome/48.0 (HighFidelityInterface limitedCommerce)" };

    const QString tokenString = !isAuthable ? tokenStringMobile : (accountManager->getLimitedCommerce() ? tokenStringLimitedCommerce : tokenStringMetaverse);
    info.setHttpHeader(USER_AGENT.toLocal8Bit(), tokenString.toLocal8Bit());
}

void RequestFilters::interceptFileType(QWebEngineUrlRequestInfo& info) {
    QString filename = info.requestUrl().fileName();
    if (isScript(filename) || isJSON(filename)) {
        static const QString CONTENT_HEADER = "Accept";
        static const QString TYPE_VALUE = "text/plain,text/html";
        info.setHttpHeader(CONTENT_HEADER.toLocal8Bit(), TYPE_VALUE.toLocal8Bit());
    }
}
#endif
