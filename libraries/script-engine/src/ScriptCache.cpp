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
#include <QNetworkConfiguration>
#include <QNetworkReply>
#include <QObject>

#include <assert.h>
#include <SharedUtil.h>

#include "ScriptCache.h"
#include "ScriptEngineLogging.h"

ScriptCache::ScriptCache(QObject* parent) {
    // nothing to do here...
}

QString ScriptCache::getScript(const QUrl& url, ScriptUser* scriptUser, bool& isPending, bool reload) {
    QString scriptContents;
    if (_scriptCache.contains(url) && !reload) {
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
            auto request = ResourceManager::createResourceRequest(this, url);
            request->setCacheEnabled(!reload);
            connect(request, &ResourceRequest::finished, this, &ScriptCache::scriptDownloaded);
            request->send();
        }
    }
    return scriptContents;
}

void ScriptCache::deleteScript(const QUrl& url) {
    if (_scriptCache.contains(url)) {
        qCDebug(scriptengine) << "Delete script from cache:" << url.toString();
        _scriptCache.remove(url);
    }
}

void ScriptCache::scriptDownloaded() {
    ResourceRequest* req = qobject_cast<ResourceRequest*>(sender());
    QUrl url = req->getUrl();
    QList<ScriptUser*> scriptUsers = _scriptUsers.values(url);
    _scriptUsers.remove(url);

    if (req->getResult() == ResourceRequest::Success) {
        _scriptCache[url] = req->getData();
        qCDebug(scriptengine) << "Done downloading script at:" << url.toString();

        foreach(ScriptUser* user, scriptUsers) {
            user->scriptContentsAvailable(url, _scriptCache[url]);
        }
    } else {
        qCWarning(scriptengine) << "Error loading script from URL " << url;
        foreach(ScriptUser* user, scriptUsers) {
            user->errorInLoadingScript(url);
        }
    }
    req->deleteLater();
}


