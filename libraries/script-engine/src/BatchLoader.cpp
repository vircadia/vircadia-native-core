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
    // QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    for (QUrl url : _urls) {
        auto request = ResourceManager::createResourceRequest(this, url);
        if (!request) {
            continue;
        }
        connect(request, &ResourceRequest::finished, [=]() {
            if (request->getResult() == ResourceRequest::SUCCESS) {
                _data.insert(url, request->getData());
            } else {
                _data.insert(url, QString());
            }
            request->deleteLater();
            checkFinished();
        });

        qCDebug(scriptengine) << "Loading script at " << url;

        request->send();

//         if (url.scheme() == "http" || url.scheme() == "https" || url.scheme() == "ftp") {
//             QNetworkRequest request = QNetworkRequest(url);
//             request.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
//             QNetworkReply* reply = networkAccessManager.get(request);
//
//             qCDebug(scriptengine) << "Downloading file at" << url;
//
//             connect(reply, &QNetworkReply::finished, [=]() {
//                 if (reply->error()) {
//                     _data.insert(url, QString());
//                 } else {
//                     _data.insert(url, reply->readAll());
//                 }
//                 reply->deleteLater();
//                 checkFinished();
//             });
//
//             // If we end up being destroyed before the reply finishes, clean it up
//             connect(this, &QObject::destroyed, reply, &QObject::deleteLater);
//
//         } else {
// #ifdef _WIN32
//             QString fileName = url.toString();
// #else
//             QString fileName = url.toLocalFile();
// #endif
//
//             qCDebug(scriptengine) << "Reading file at " << fileName;
//
//             QFile scriptFile(fileName);
//             if (scriptFile.open(QFile::ReadOnly | QFile::Text)) {
//                 QTextStream in(&scriptFile);
//                 _data.insert(url, in.readAll());
//             } else {
//                 _data.insert(url, QString());
//             }
//         }
    }
    checkFinished();
}

void BatchLoader::checkFinished() {
    if (!_finished && _urls.size() == _data.size()) {
        _finished = true;
        emit finished(_data);
    }
}
