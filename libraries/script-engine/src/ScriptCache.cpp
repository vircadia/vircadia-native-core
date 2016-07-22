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

#include "ScriptCache.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkConfiguration>
#include <QNetworkReply>
#include <QObject>
#include <QThread>

#include <assert.h>
#include <SharedUtil.h>

#include "ScriptEngines.h"
#include "ScriptEngineLogging.h"

ScriptCache::ScriptCache(QObject* parent) {
    // nothing to do here...
}

void ScriptCache::clearCache() {
    Lock lock(_containerLock);
    _scriptCache.clear();
}

QString ScriptCache::getScript(const QUrl& unnormalizedURL, ScriptUser* scriptUser, bool& isPending, bool reload) {
    QUrl url = ResourceManager::normalizeURL(unnormalizedURL);
    QString scriptContents;

    Lock lock(_containerLock);
    if (_scriptCache.contains(url) && !reload) {
        qCDebug(scriptengine) << "Found script in cache:" << url.toString();
        scriptContents = _scriptCache[url];
        lock.unlock();
        scriptUser->scriptContentsAvailable(url, scriptContents);
        isPending = false;
    } else {
        isPending = true;
        bool alreadyWaiting = _scriptUsers.contains(url);
        _scriptUsers.insert(url, scriptUser);
        lock.unlock();

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
    Lock lock(_containerLock);
    if (_scriptCache.contains(url)) {
        qCDebug(scriptengine) << "Delete script from cache:" << url.toString();
        _scriptCache.remove(url);
    }
}

void ScriptCache::scriptDownloaded() {
    ResourceRequest* req = qobject_cast<ResourceRequest*>(sender());
    QUrl url = req->getUrl();

    Lock lock(_containerLock);
    QList<ScriptUser*> scriptUsers = _scriptUsers.values(url);
    _scriptUsers.remove(url);

    if (!DependencyManager::get<ScriptEngines>()->isStopped()) {
        if (req->getResult() == ResourceRequest::Success) {
            auto scriptContents = req->getData();
            _scriptCache[url] = scriptContents;
            lock.unlock();
            qCDebug(scriptengine) << "Done downloading script at:" << url.toString();

            foreach(ScriptUser* user, scriptUsers) {
                user->scriptContentsAvailable(url, scriptContents);
            }
        } else {
            lock.unlock();
            qCWarning(scriptengine) << "Error loading script from URL " << url;
            foreach(ScriptUser* user, scriptUsers) {
                user->errorInLoadingScript(url);
            }
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

    Lock lock(_containerLock);
    if (_scriptCache.contains(url) && !forceDownload) {
        auto scriptContent = _scriptCache[url];
        lock.unlock();
        qCDebug(scriptengine) << "Found script in cache:" << url.toString();
        contentAvailable(url.toString(), scriptContent, true, true);
    } else {
        bool alreadyWaiting = _contentCallbacks.contains(url);
        _contentCallbacks.insert(url, contentAvailable);
        lock.unlock();

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

    QString scriptContent;
    QList<contentAvailableCallback> allCallbacks;
    bool success { false };

    {
        Lock lock(_containerLock);
        allCallbacks = _contentCallbacks.values(url);
        _contentCallbacks.remove(url);
        success = req->getResult() == ResourceRequest::Success;

        if (success) {
            _scriptCache[url] = scriptContent = req->getData();
            qCDebug(scriptengine) << "Done downloading script at:" << url.toString();
        } else {
            // Dubious, but retained here because it matches the behavior before fixing the threading
            scriptContent = _scriptCache[url];
            qCWarning(scriptengine) << "Error loading script from URL " << url;
        }
    }

    req->deleteLater();

    if (!DependencyManager::get<ScriptEngines>()->isStopped()) {
        foreach(contentAvailableCallback thisCallback, allCallbacks) {
            thisCallback(url.toString(), scriptContent, true, success);
        }
    }
}
