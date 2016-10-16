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
      _data() {
    qRegisterMetaType<QMap<QUrl, QString>>("QMap<QUrl, QString>");
}

void BatchLoader::start() {
    if (_started) {
        return;
    }

    _started = true;

    for (const auto& rawURL : _urls) {
        QUrl url = expandScriptUrl(normalizeScriptURL(rawURL));

        qCDebug(scriptengine) << "Loading script at " << url;

        QPointer<BatchLoader> self = this;
        DependencyManager::get<ScriptCache>()->getScriptContents(url.toString(), [this, self](const QString& url, const QString& contents, bool isURL, bool success) {
            if (!self) {
                return;
            }

            // Because the ScriptCache may call this callback from differents threads,
            // we need to make sure this is thread-safe.
            std::lock_guard<std::mutex> lock(_dataLock);

            if (isURL && success) {
                _data.insert(url, contents);
                qCDebug(scriptengine) << "Loaded: " << url;
            } else {
                _data.insert(url, QString());
                qCDebug(scriptengine) << "Could not load" << url;
            }

            if (!_finished && _urls.size() == _data.size()) {
                _finished = true;
                emit finished(_data);
            }
        }, false);
    }
}
