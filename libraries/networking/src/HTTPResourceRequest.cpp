//
//  HTTPResourceRequest.cpp
//  libraries/networking/src
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
    Q_ASSERT(_state == InProgress);
    Q_ASSERT(_reply);

    _state = Finished;

    auto error = _reply->error();
    if (error == QNetworkReply::NoError) {
        _data = _reply->readAll();
        _loadedFromCache = _reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool();
        _result = ResourceRequest::Success;
        emit finished();
    } else if (error == QNetworkReply::TimeoutError) {
        _result = ResourceRequest::Timeout;
        emit finished();
    } else {
        _result = ResourceRequest::Error;
        emit finished();
    }

    _reply->deleteLater();
    _reply = nullptr;
}

void HTTPResourceRequest::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (_state == InProgress) {
        // We've received data, so reset the timer
        _sendTimer.start();
    }

    emit progress(bytesReceived, bytesTotal);
}

void HTTPResourceRequest::onTimeout() {
    Q_ASSERT(_state != Unsent);

    if (_state == InProgress) {
        qCDebug(networking) << "Timed out loading " << _url;
        _reply->abort();
        _state = Finished;
        _result = Timeout;
        emit finished();
    }
}
