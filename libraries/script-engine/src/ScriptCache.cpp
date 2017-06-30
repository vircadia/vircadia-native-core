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
#include <QRegularExpression>
#include <QMetaEnum>

#include <assert.h>
#include <SharedUtil.h>

#include "ScriptEngines.h"
#include "ScriptEngineLogging.h"
#include <QtCore/QTimer>

const QString ScriptCache::STATUS_INLINE { "Inline" };
const QString ScriptCache::STATUS_CACHED { "Cached" };

ScriptCache::ScriptCache(QObject* parent) {
    // nothing to do here...
}

void ScriptCache::clearCache() {
    Lock lock(_containerLock);
    foreach(auto& url, _scriptCache.keys()) {
        qCDebug(scriptengine) << "clearing cache: " << url;
    }
    _scriptCache.clear();
}

void ScriptCache::clearATPScriptsFromCache() {
    Lock lock(_containerLock);
    qCDebug(scriptengine) << "Clearing ATP scripts from ScriptCache";
    for (auto it = _scriptCache.begin(); it != _scriptCache.end();) {
        if (it.key().scheme() == "atp") {
            qCDebug(scriptengine) << "Removing: " << it.key();
            it = _scriptCache.erase(it);
        } else {
            ++it;
        }
    }
}

void ScriptCache::deleteScript(const QUrl& unnormalizedURL) {
    QUrl url = DependencyManager::get<ResourceManager>()->normalizeURL(unnormalizedURL);
    Lock lock(_containerLock);
    if (_scriptCache.contains(url)) {
        qCDebug(scriptengine) << "Delete script from cache:" << url.toString();
        _scriptCache.remove(url);
    }
}

void ScriptCache::getScriptContents(const QString& scriptOrURL, contentAvailableCallback contentAvailable, bool forceDownload, int maxRetries) {
    #ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptCache::getScriptContents() on thread [" << QThread::currentThread() << "] expected thread [" << thread() << "]";
    #endif
    QUrl unnormalizedURL(scriptOrURL);
    QUrl url = DependencyManager::get<ResourceManager>()->normalizeURL(unnormalizedURL);

    // attempt to determine if this is a URL to a script, or if this is actually a script itself (which is valid in the
    // entityScript use case)
    if (unnormalizedURL.scheme().isEmpty() &&
            scriptOrURL.simplified().replace(" ", "").contains(QRegularExpression(R"(\(function\([a-z]?[\w,]*\){)"))) {
        contentAvailable(scriptOrURL, scriptOrURL, false, true, STATUS_INLINE);
        return;
    }

    // give a similar treatment to javacript: urls
    if (unnormalizedURL.scheme() == "javascript") {
        QString contents { scriptOrURL };
        contents.replace(QRegularExpression("^javascript:"), "");
        contentAvailable(scriptOrURL, contents, false, true, STATUS_INLINE);
        return;
    }

    Lock lock(_containerLock);
    if (_scriptCache.contains(url) && !forceDownload) {
        auto scriptContent = _scriptCache[url];
        lock.unlock();
        qCDebug(scriptengine) << "Found script in cache:" << url.toString();
        contentAvailable(url.toString(), scriptContent, true, true, STATUS_CACHED);
    } else {
        auto& scriptRequest = _activeScriptRequests[url];
        bool alreadyWaiting = scriptRequest.scriptUsers.size() > 0;
        scriptRequest.scriptUsers.push_back(contentAvailable);

        lock.unlock();

        if (alreadyWaiting) {
            qCDebug(scriptengine) << QString("Already downloading script at: %1 (retry: %2; scriptusers: %3)")
                .arg(url.toString()).arg(scriptRequest.numRetries).arg(scriptRequest.scriptUsers.size());
        } else {
            scriptRequest.maxRetries = maxRetries;
            #ifdef THREAD_DEBUGGING
            qCDebug(scriptengine) << "about to call: ResourceManager::createResourceRequest(this, url); on thread [" << QThread::currentThread() << "] expected thread [" << thread() << "]";
            #endif
            auto request = DependencyManager::get<ResourceManager>()->createResourceRequest(nullptr, url);
            Q_ASSERT(request);
            request->setCacheEnabled(!forceDownload);
            connect(request, &ResourceRequest::finished, this, [=]{ scriptContentAvailable(maxRetries); });
            request->send();
        }
    }
}

void ScriptCache::scriptContentAvailable(int maxRetries) {
    #ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptCache::scriptContentAvailable() on thread [" << QThread::currentThread() << "] expected thread [" << thread() << "]";
    #endif
    ResourceRequest* req = qobject_cast<ResourceRequest*>(sender());
    QUrl url = req->getUrl();

    QString scriptContent;
    std::vector<contentAvailableCallback> allCallbacks;
    QString status = QMetaEnum::fromType<ResourceRequest::Result>().valueToKey(req->getResult());
    bool success { false };

    {
        Q_ASSERT(req->getState() == ResourceRequest::Finished);
        success = req->getResult() == ResourceRequest::Success;

        Lock lock(_containerLock);

        if (_activeScriptRequests.contains(url)) {
            auto& scriptRequest = _activeScriptRequests[url];

            if (success) {
                allCallbacks = scriptRequest.scriptUsers;

                _activeScriptRequests.remove(url);

                _scriptCache[url] = scriptContent = req->getData();
                qCDebug(scriptengine) << "Done downloading script at:" << url.toString();
            } else {
                auto result = req->getResult();
                bool irrecoverable =
                    result == ResourceRequest::AccessDenied ||
                    result == ResourceRequest::InvalidURL ||
                    result == ResourceRequest::NotFound ||
                    scriptRequest.numRetries >= maxRetries;

                if (!irrecoverable) {
                    ++scriptRequest.numRetries;

                    int timeout = exp(scriptRequest.numRetries) * ScriptRequest::START_DELAY_BETWEEN_RETRIES;
                    int attempt = scriptRequest.numRetries;
                    qCDebug(scriptengine) << QString("Script request failed [%1]: %2 (will retry %3 more times; attempt #%4 in %5ms...)")
                        .arg(status).arg(url.toString()).arg(maxRetries - attempt + 1).arg(attempt).arg(timeout);

                    QTimer::singleShot(timeout, this, [this, url, attempt, maxRetries]() {
                        qCDebug(scriptengine) << QString("Retrying script request [%1 / %2]: %3")
                            .arg(attempt).arg(maxRetries).arg(url.toString());

                        auto request = DependencyManager::get<ResourceManager>()->createResourceRequest(nullptr, url);
                        Q_ASSERT(request);

                        // We've already made a request, so the cache must be disabled or it wasn't there, so enabling
                        // it will do nothing.
                        request->setCacheEnabled(false);
                        connect(request, &ResourceRequest::finished, this, [=]{ scriptContentAvailable(maxRetries); });
                        request->send();
                    });
                } else {
                    // Dubious, but retained here because it matches the behavior before fixing the threading

                    allCallbacks = scriptRequest.scriptUsers;

                    if (_scriptCache.contains(url)) {
                        scriptContent = _scriptCache[url];
                    }
                    _activeScriptRequests.remove(url);
                    qCWarning(scriptengine) << "Error loading script from URL " << url << "(" << status <<")";

                }
            }
        } else {
            qCWarning(scriptengine) << "Warning: scriptContentAvailable for inactive url: " << url;
        }
    }

    req->deleteLater();

    if (allCallbacks.size() > 0 && !DependencyManager::get<ScriptEngines>()->isStopped()) {
        foreach(contentAvailableCallback thisCallback, allCallbacks) {
            thisCallback(url.toString(), scriptContent, true, success, status);
        }
    }
}
