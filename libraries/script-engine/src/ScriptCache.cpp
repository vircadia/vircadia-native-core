//
//  ScriptCache.cpp
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 2015-03-30
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QCoreApplication>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>

#include <NetworkAccessManager.h>
#include <SharedUtil.h>
#include "ScriptEngineLogging.h"
#include "ScriptCache.h"

ScriptCache::ScriptCache(QObject* parent) {
    // nothing to do here...
}

QString ScriptCache::getScript(const QUrl& url, ScriptUser* scriptUser, bool& isPending) {
    QString scriptContents;
    if (_scriptCache.contains(url)) {
        qCDebug(scriptengine) << "Found script in cache:" << url.toString();
        scriptContents = _scriptCache[url];
        scriptUser->scriptContentsAvailable(url, scriptContents);
        isPending = false;
    } else {
        isPending = true;
        bool alreadyWaiting = _scriptUsers.contains(url);
        _scriptUsers.insert(url, scriptUser);
        
        if (alreadyWaiting) {
            qCDebug(scriptengine) << "Already downloading script at:" << url.toString();
        } else {
            QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
            QNetworkRequest networkRequest = QNetworkRequest(url);
            networkRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);

            qCDebug(scriptengine) << "Downloading script at:" << url.toString();
            QNetworkReply* reply = networkAccessManager.get(networkRequest);
            connect(reply, &QNetworkReply::finished, this, &ScriptCache::scriptDownloaded);
        }
    }
    return scriptContents;
}

void ScriptCache::scriptDownloaded() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    QUrl url = reply->url();
    QList<ScriptUser*> scriptUsers = _scriptUsers.values(url);
    _scriptUsers.remove(url);
    
    if (reply->error() == QNetworkReply::NoError && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200) {
        _scriptCache[url] = reply->readAll();
        qCDebug(scriptengine) << "Done downloading script at:" << url.toString();

        foreach(ScriptUser* user, scriptUsers) {
            user->scriptContentsAvailable(url, _scriptCache[url]);
        }
    } else {
        qCDebug(scriptengine) << "ERROR Loading file:" << reply->url().toString();
        foreach(ScriptUser* user, scriptUsers) {
            user->errorInLoadingScript(url);
        }
    }
    reply->deleteLater();
}

    
