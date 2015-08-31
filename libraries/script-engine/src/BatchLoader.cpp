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
#include "ScriptEngineLogging.h"
#include "BatchLoader.h"
#include <NetworkAccessManager.h>
#include <SharedUtil.h>
#include "ResourceManager.h"

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
    for (const auto& url : _urls) {
        auto request = ResourceManager::createResourceRequest(this, url);
        if (!request) {
            _data.insert(url, QString());
            qCDebug(scriptengine) << "Could not load" << url;
            continue;
        }
        connect(request, &ResourceRequest::finished, this, [=]() {
            if (request->getResult() == ResourceRequest::Success) {
                _data.insert(url, request->getData());
            } else {
                _data.insert(url, QString());
                qCDebug(scriptengine) << "Could not load" << url;
            }
            request->deleteLater();
            checkFinished();
        });

        // If we end up being destroyed before the reply finishes, clean it up
        connect(this, &QObject::destroyed, request, &QObject::deleteLater);

        qCDebug(scriptengine) << "Loading script at " << url;

        request->send();
    }
    checkFinished();
}

void BatchLoader::checkFinished() {
    if (!_finished && _urls.size() == _data.size()) {
        _finished = true;
        emit finished(_data);
    }
}
