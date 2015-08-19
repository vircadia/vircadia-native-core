//
//  ResourceManager.cpp
//
//  Created by Ryan Huffman on 2015/07/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ResourceManager.h"

#include <QFile>
#include <QNetworkRequest>

#include "AssetClient.h"
#include "AssetRequest.h"
#include "NetworkAccessManager.h"

#include <SharedUtil.h>

const QString URL_SCHEME_FILE = "file";
const QString URL_SCHEME_HTTP = "http";
const QString URL_SCHEME_HTTPS = "https";
const QString URL_SCHEME_FTP = "ftp";
const QString URL_SCHEME_ATP = "atp";

ResourceRequest::ResourceRequest(QObject* parent, const QUrl& url) :
    QObject(parent),
    _url(url) {
}

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
    QObject::connect(_reply, &QNetworkReply::finished, [=]() {
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
    });
}

void FileResourceRequest::doSend() {
    QString filename = _url.toLocalFile();
    QFile file(filename);
    _state = FINISHED;
    if (file.open(QFile::ReadOnly)) {
        _data = file.readAll();
        _result = ResourceRequest::SUCCESS;
        emit finished();
    } else {
        _result = ResourceRequest::ERROR;
        emit finished();
    }
}

void ATPResourceRequest::doSend() {
    // Make request to atp
    auto assetClient = DependencyManager::get<AssetClient>();
    auto hash = _url.path();

    auto request = assetClient->create(hash);

    if (!request) {
        return;
    }

    QObject::connect(request, &AssetRequest::finished, [this](AssetRequest* req) mutable {
        if (_state != IN_PROGRESS) return;
        _state = FINISHED;
        if (true) {
            _data = req->getData();
            _result = ResourceRequest::SUCCESS;
            emit finished();
        } else {
            _result = ResourceRequest::ERROR;
            emit finished();
        }
    });

    request->start();
}

const int TIMEOUT_MS = 2000;
void ResourceRequest::send() {
    Q_ASSERT(_state == UNSENT);

    _state = IN_PROGRESS;
    doSend();
    connect(&_sendTimer, &QTimer::timeout, this, &ResourceRequest::timeout);
    _sendTimer.setSingleShot(true);
    _sendTimer.start(TIMEOUT_MS);
}

void ResourceRequest::timeout() {
    Q_ASSERT(_state != UNSENT);

    // TIMEOUT!!
    if (_state == IN_PROGRESS) {
        qDebug() << "TImed out loading " << _url;
        _state = FINISHED;
        _result = TIMEOUT;
        emit finished();
    }
}

ResourceRequest* ResourceManager::createResourceRequest(QObject* parent, const QUrl& url) {
    auto scheme = url.scheme();
    if (scheme == URL_SCHEME_FILE) {
        return new FileResourceRequest(parent, url);
    } else if (scheme == URL_SCHEME_HTTP || scheme == URL_SCHEME_HTTPS || scheme == URL_SCHEME_FTP) {
        return new HTTPResourceRequest(parent, url);
    } else if (scheme == URL_SCHEME_ATP) {
        return new ATPResourceRequest(parent, url);
    }

    qDebug() << "Failed to load: " << url.url();

    return nullptr;
}
