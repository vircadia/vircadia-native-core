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
#include <QMetaEnum>

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

void HTTPResourceRequest::setupTimer() {
    Q_ASSERT(!_sendTimer);
    static const int TIMEOUT_MS = 10000;

    _sendTimer = new QTimer();
    connect(this, &QObject::destroyed, _sendTimer, &QTimer::deleteLater);
    connect(_sendTimer, &QTimer::timeout, this, &HTTPResourceRequest::onTimeout);

    _sendTimer->setSingleShot(true);
    _sendTimer->start(TIMEOUT_MS);
}

void HTTPResourceRequest::cleanupTimer() {
    Q_ASSERT(_sendTimer);
    _sendTimer->disconnect(this);
    _sendTimer->deleteLater();
    _sendTimer = nullptr;
}

void HTTPResourceRequest::doSend() {
    QNetworkRequest networkRequest(_url);
    networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);

    if (_cacheEnabled) {
        networkRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    } else {
        networkRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    }

    _reply = NetworkAccessManager::getInstance().get(networkRequest);
    
    connect(_reply, &QNetworkReply::finished, this, &HTTPResourceRequest::onRequestFinished);
    connect(_reply, &QNetworkReply::downloadProgress, this, &HTTPResourceRequest::onDownloadProgress);

    setupTimer();
}

void HTTPResourceRequest::onRequestFinished() {
    Q_ASSERT(_state == InProgress);
    Q_ASSERT(_reply);

    cleanupTimer();
    
    switch(_reply->error()) {
        case QNetworkReply::NoError:
            _data = _reply->readAll();
            _loadedFromCache = _reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool();
            _result = Success;
            break;

        case QNetworkReply::TimeoutError:
            _result = Timeout;
            break;

        case QNetworkReply::ContentNotFoundError: // Script.include('https://httpbin.org/status/404')
            _result = NotFound;
            break;

        case QNetworkReply::ProtocolInvalidOperationError: // Script.include('https://httpbin.org/status/400')
            _result = InvalidURL;
            break;

        case QNetworkReply::UnknownContentError: // Script.include('QUrl("https://httpbin.org/status/402")')
        case QNetworkReply::ContentOperationNotPermittedError: //Script.include('https://httpbin.org/status/403')
        case QNetworkReply::AuthenticationRequiredError: // Script.include('https://httpbin.org/basic-auth/user/passwd')
            _result = AccessDenied;
            break;

        case QNetworkReply::RemoteHostClosedError:  // Script.include('http://127.0.0.1:22')
        case QNetworkReply::ConnectionRefusedError: // Script.include(http://127.0.0.1:1')
        case QNetworkReply::HostNotFoundError:      // Script.include('http://foo.bar.highfidelity.io')
        case QNetworkReply::ServiceUnavailableError: // Script.include('QUrl("https://httpbin.org/status/503")')
            _result = ServerUnavailable;
            break;

        case QNetworkReply::UnknownServerError: // Script.include('QUrl("https://httpbin.org/status/504")')
        case QNetworkReply::InternalServerError: // Script.include('QUrl("https://httpbin.org/status/500")')
        default:
            qDebug() << "HTTPResourceRequest error:" << QMetaEnum::fromType<QNetworkReply::NetworkError>().valueToKey(_reply->error());
            _result = Error;
            break;
    }
    _reply->disconnect(this);
    _reply->deleteLater();
    _reply = nullptr;
    
    _state = Finished;
    emit finished();
}

void HTTPResourceRequest::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    Q_ASSERT(_state == InProgress);
    
    // We've received data, so reset the timer
    _sendTimer->start();

    emit progress(bytesReceived, bytesTotal);
}

void HTTPResourceRequest::onTimeout() {
    Q_ASSERT(_state == InProgress);
    _reply->disconnect(this);
    _reply->abort();
    _reply->deleteLater();
    _reply = nullptr;

    cleanupTimer();
    
    _result = Timeout;
    _state = Finished;
    emit finished();
}
