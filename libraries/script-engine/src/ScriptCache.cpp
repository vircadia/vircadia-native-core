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
#include <QThread>

#include <assert.h>
#include <SharedUtil.h>

#include "ScriptCache.h"
#include "ScriptEngineLogging.h"

ScriptCache::ScriptCache(QObject* parent) {
    // nothing to do here...
}

void ScriptCache::clearCache() {
    _scriptCache.clear();
}

QString ScriptCache::getScript(const QUrl& unnormalizedURL, ScriptUser* scriptUser, bool& isPending, bool reload) {
    QUrl url = ResourceManager::normalizeURL(unnormalizedURL);
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
            auto request = ResourceManager::createResourceRequest(nullptr, url);
            request->setCacheEnabled(!reload);
            connect(request, &ResourceRequest::finished, this, &ScriptCache::scriptDownloaded);
            request->send();
        }
    }
    return scriptContents;
}

void ScriptCache::deleteScript(const QUrl& unnormalizedURL) {
    QUrl url = ResourceManager::normalizeURL(unnormalizedURL);
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

void ScriptCache::getScriptContents(const QString& scriptOrURL, contentAvailableCallback contentAvailable, bool forceDownload) {
    #ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptCache::getScriptContents() on thread [" << QThread::currentThread() << "] expected thread [" << thread() << "]";
    #endif
    QUrl unnormalizedURL(scriptOrURL);
    QUrl url = ResourceManager::normalizeURL(unnormalizedURL);

    // attempt to determine if this is a URL to a script, or if this is actually a script itself (which is valid in the entityScript use case)
    if (url.scheme().isEmpty() && scriptOrURL.simplified().replace(" ", "").contains("(function(){")) {
        contentAvailable(scriptOrURL, scriptOrURL, false, true);
        return;
    }

    if (_scriptCache.contains(url) && !forceDownload) {
        qCDebug(scriptengine) << "Found script in cache:" << url.toString();
        #if 1 // def THREAD_DEBUGGING
        qCDebug(scriptengine) << "ScriptCache::getScriptContents() about to call contentAvailable() on thread [" << QThread::currentThread() << "] expected thread [" << thread() << "]";
        #endif
        contentAvailable(url.toString(), _scriptCache[url], true, true);
    } else {
        bool alreadyWaiting = _contentCallbacks.contains(url);
        _contentCallbacks.insert(url, contentAvailable);

        if (alreadyWaiting) {
            qCDebug(scriptengine) << "Already downloading script at:" << url.toString();
        } else {
            #ifdef THREAD_DEBUGGING
            qCDebug(scriptengine) << "about to call: ResourceManager::createResourceRequest(this, url); on thread [" << QThread::currentThread() << "] expected thread [" << thread() << "]";
            #endif
            auto request = ResourceManager::createResourceRequest(nullptr, url);
            request->setCacheEnabled(!forceDownload);
            connect(request, &ResourceRequest::finished, this, &ScriptCache::scriptContentAvailable);
            request->send();
        }
    }
}

void ScriptCache::scriptContentAvailable() {
    #ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptCache::scriptContentAvailable() on thread [" << QThread::currentThread() << "] expected thread [" << thread() << "]";
    #endif
    ResourceRequest* req = qobject_cast<ResourceRequest*>(sender());
    QUrl url = req->getUrl();
    QList<contentAvailableCallback> allCallbacks = _contentCallbacks.values(url);
    _contentCallbacks.remove(url);

    bool success = req->getResult() == ResourceRequest::Success;

    if (success) {
        _scriptCache[url] = req->getData();
        qCDebug(scriptengine) << "Done downloading script at:" << url.toString();
    } else {
        qCWarning(scriptengine) << "Error loading script from URL " << url;
    }
    foreach(contentAvailableCallback thisCallback, allCallbacks) {
        thisCallback(url.toString(), _scriptCache[url], true, success);
    }
    req->deleteLater();
}

