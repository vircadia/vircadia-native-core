//
//  QmlAtpReply.cpp
//  libraries/networking/src
//
//  Created by Zander Otavka on 8/4/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ResourceManager.h"
#include "QmlAtpReply.h"

QmlAtpReply::QmlAtpReply(const QUrl& url, QObject* parent) :
        _resourceRequest(ResourceManager::createResourceRequest(parent, url)) {
    setOperation(QNetworkAccessManager::GetOperation);

    connect(_resourceRequest, &AssetResourceRequest::progress, this, &QmlAtpReply::downloadProgress);
    connect(_resourceRequest, &AssetResourceRequest::finished, this, &QmlAtpReply::handleRequestFinish);

    _resourceRequest->send();
}

QmlAtpReply::~QmlAtpReply() {
    if (_resourceRequest) {
        _resourceRequest->deleteLater();
        _resourceRequest = nullptr;
    }
}

qint64 QmlAtpReply::bytesAvailable() const {
    return _content.size() - _readOffset + QIODevice::bytesAvailable();
}

qint64 QmlAtpReply::readData(char* data, qint64 maxSize) {
    if (_readOffset < _content.size()) {
        qint64 readSize = qMin(maxSize, _content.size() - _readOffset);
        memcpy(data, _content.constData() + _readOffset, readSize);
        _readOffset += readSize;
        return readSize;
    } else {
        return -1;
    }
}

void QmlAtpReply::handleRequestFinish() {
    Q_ASSERT(_resourceRequest->getState() == ResourceRequest::State::Finished);

    switch (_resourceRequest->getResult()) {
        case ResourceRequest::Result::Success:
            setError(NoError, "Success");
            _content = _resourceRequest->getData();
            break;
        case ResourceRequest::Result::InvalidURL:
            setError(ContentNotFoundError, "Invalid URL");
            break;
        case ResourceRequest::Result::NotFound:
            setError(ContentNotFoundError, "Not found");
            break;
        case ResourceRequest::Result::ServerUnavailable:
            setError(ServiceUnavailableError, "Service unavailable");
            break;
        case ResourceRequest::Result::AccessDenied:
            setError(ContentAccessDenied, "Access denied");
            break;
        case ResourceRequest::Result::Timeout:
            setError(TimeoutError, "Timeout");
            break;
        default:
            setError(UnknownNetworkError, "Unknown error");
            break;
    }

    open(ReadOnly | Unbuffered);
    setHeader(QNetworkRequest::ContentLengthHeader, QVariant(_content.size()));

    if (error() != NoError) {
        emit error(error());
    }

    setFinished(true);
    emit readyRead();
    emit finished();

    _resourceRequest->deleteLater();
    _resourceRequest = nullptr;
}
