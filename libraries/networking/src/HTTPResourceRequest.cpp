//
//  HTTPResourceRequest.cpp
//
//  Created by Ryan Huffman on 2015/07/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HTTPResourceRequest.h"

#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <SharedUtil.h>

#include "NetworkAccessManager.h"
#include "NetworkLogging.h"

HTTPResourceRequest::~HTTPResourceRequest() {
    if (_reply) {
        _reply->disconnect(this);
        _reply->deleteLater();
        _reply = nullptr;
    }
}

void HTTPResourceRequest::doSend() {
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkRequest networkRequest = QNetworkRequest(_url);
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);

    if (_cacheEnabled) {
        networkRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    } else {
        networkRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    } 

    _reply = networkAccessManager.get(networkRequest);

    connect(_reply, &QNetworkReply::finished, this, &HTTPResourceRequest::onRequestFinished);

    static const int TIMEOUT_MS = 10000;

    connect(&_sendTimer, &QTimer::timeout, this, &HTTPResourceRequest::onTimeout);
    _sendTimer.setSingleShot(true);
    _sendTimer.start(TIMEOUT_MS);
}

void HTTPResourceRequest::onRequestFinished() {
    Q_ASSERT(_state == IN_PROGRESS);
    Q_ASSERT(_reply);

    _state = FINISHED;
    auto error = _reply->error();
    qDebug() << "Loaded " << _url;
    QString u = _url.path();
    if (error == QNetworkReply::NoError) {
        _data = _reply->readAll();
        qDebug() << "!!!! " << _data.size() << " " << _url.path();
        _loadedFromCache = _reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool();
        _result = ResourceRequest::SUCCESS;
        emit finished();
    } else if (error == QNetworkReply::TimeoutError) {
        _result = ResourceRequest::TIMEOUT;
        emit finished();
    } else {
        _result = ResourceRequest::ERROR;
        emit finished();
    }
    _reply->deleteLater();
    _reply = nullptr;
}

void HTTPResourceRequest::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (_state == IN_PROGRESS) {
        // Restart timer
        _sendTimer.start();
    }
}

void HTTPResourceRequest::onTimeout() {
    Q_ASSERT(_state != UNSENT);

    // TODO Cancel request if timed out, handle properly in
    // receive callback
    if (_state == IN_PROGRESS) {
        qCDebug(networking) << "Timed out loading " << _url;
        _state = FINISHED;
        _result = TIMEOUT;
        _reply->abort();
        emit finished();
    }
}
