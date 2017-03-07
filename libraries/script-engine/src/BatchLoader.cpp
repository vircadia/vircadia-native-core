//
//  BatchLoader.cpp
//  libraries/script-engine/src
//
//  Created by Ryan Huffman on 01/22/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <QFile>
#include <QPointer>
#include "ScriptEngineLogging.h"
#include "BatchLoader.h"
#include <NetworkAccessManager.h>
#include <SharedUtil.h>
#include "ResourceManager.h"
#include "ScriptEngines.h"
#include "ScriptCache.h"

BatchLoader::BatchLoader(const QList<QUrl>& urls) 
    : QObject(),
      _started(false),
      _finished(false),
      _urls(urls.toSet()),
      _data(),
      _status() {
    qRegisterMetaType<QMap<QUrl, QString>>("QMap<QUrl, QString>");
}

void BatchLoader::start(int maxRetries) {
    if (_started) {
        return;
    }

    _started = true;

    if (_urls.size() == 0) {
        _finished = true;
        emit finished(_data, _status);
        return;
    }


    for (const auto& rawURL : _urls) {
        QUrl url = expandScriptUrl(normalizeScriptURL(rawURL));

        qCDebug(scriptengine) << "Loading script at " << url;

        auto scriptCache = DependencyManager::get<ScriptCache>();

        // Use a proxy callback to handle the call and emit the signal in a thread-safe way.
        // If BatchLoader is deleted before the callback is called, the subsequent "emit" call will not do
        // anything.
        ScriptCacheSignalProxy* proxy = new ScriptCacheSignalProxy();
        connect(scriptCache.data(), &ScriptCache::destroyed, proxy, &ScriptCacheSignalProxy::deleteLater);

        connect(proxy, &ScriptCacheSignalProxy::contentAvailable, this, [this](const QString& url, const QString& contents, bool isURL, bool success, const QString& status) {
            _status.insert(url, status);
            if (isURL && success) {
                _data.insert(url, contents);
                qCDebug(scriptengine) << "Loaded: " << url;
            } else {
                _data.insert(url, QString());
                qCDebug(scriptengine) << "Could not load: " << url << status;
            }

            if (!_finished && _urls.size() == _data.size()) {
                _finished = true;
                emit finished(_data, _status);
            }
        });

        scriptCache->getScriptContents(url.toString(), [proxy](const QString& url, const QString& contents, bool isURL, bool success, const QString& status) {
            proxy->receivedContent(url, contents, isURL, success, status);
            proxy->deleteLater();
        }, false, maxRetries);
    }
}

void ScriptCacheSignalProxy::receivedContent(const QString& url, const QString& contents, bool isURL, bool success, const QString& status) {
    emit contentAvailable(url, contents, isURL, success, status);
}
