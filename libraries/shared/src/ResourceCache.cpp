//
//  ResourceCache.cpp
//  shared
//
//  Created by Andrzej Kapolka on 2/27/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include <cfloat>
#include <cmath>

#include <QNetworkReply>
#include <QTimer>
#include <QtDebug>

#include "ResourceCache.h"

ResourceCache::ResourceCache(QObject* parent) :
    QObject(parent) {
}

QSharedPointer<Resource> ResourceCache::getResource(const QUrl& url, const QUrl& fallback, bool delayLoad, void* extra) {
    if (!url.isValid() && fallback.isValid()) {
        return getResource(fallback, QUrl(), delayLoad);
    }
    QSharedPointer<Resource> resource = _resources.value(url);
    if (resource.isNull()) {
        resource = createResource(url, fallback.isValid() ?
            getResource(fallback, QUrl(), true) : QSharedPointer<Resource>(), delayLoad, extra);
        _resources.insert(url, resource);
    }
    return resource;
}

void ResourceCache::attemptRequest(Resource* resource) {
    if (_requestLimit <= 0) {
        // wait until a slot becomes available
        _pendingRequests.append(resource);
        return;
    }
    _requestLimit--;
    resource->makeRequest();
}

void ResourceCache::requestCompleted() {
    _requestLimit++;
    
    // look for the highest priority pending request
    int highestIndex = -1;
    float highestPriority = -FLT_MAX;
    for (int i = 0; i < _pendingRequests.size(); ) {
        Resource* resource = _pendingRequests.at(i).data();
        if (!resource) {
            _pendingRequests.removeAt(i);
            continue;
        }
        float priority = resource->getLoadPriority();
        if (priority >= highestPriority) {
            highestPriority = priority;
            highestIndex = i;
        }
        i++;
    }
    if (highestIndex >= 0) {
        attemptRequest(_pendingRequests.takeAt(highestIndex));
    }
}

QNetworkAccessManager* ResourceCache::_networkAccessManager = NULL;

const int DEFAULT_REQUEST_LIMIT = 10;
int ResourceCache::_requestLimit = DEFAULT_REQUEST_LIMIT;

QList<QPointer<Resource> > ResourceCache::_pendingRequests;

Resource::Resource(const QUrl& url, bool delayLoad) :
    _request(url),
    _startedLoading(false),
    _failedToLoad(false),
    _loaded(false),
    _attempts(0),
    _reply(NULL) {
    
    if (!url.isValid()) {
        _startedLoading = _failedToLoad = true;
        return;
    }
    _request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    
    // start loading immediately unless instructed otherwise
    if (!delayLoad) {    
        attemptRequest();
    }
}

Resource::~Resource() {
    if (_reply) {
        ResourceCache::requestCompleted();
        delete _reply;
    }
}

void Resource::ensureLoading() {
    if (!_startedLoading) {
        attemptRequest();
    }
}

void Resource::setLoadPriority(const QPointer<QObject>& owner, float priority) {
    if (!(_failedToLoad || _loaded)) {
        _loadPriorities.insert(owner, priority);
    }
}

void Resource::setLoadPriorities(const QHash<QPointer<QObject>, float>& priorities) {
    if (_failedToLoad || _loaded) {
        return;
    }
    for (QHash<QPointer<QObject>, float>::const_iterator it = priorities.constBegin();
            it != priorities.constEnd(); it++) {
        _loadPriorities.insert(it.key(), it.value());
    }
}

void Resource::clearLoadPriority(const QPointer<QObject>& owner) {
    if (!(_failedToLoad || _loaded)) {
        _loadPriorities.remove(owner);
    }
}

float Resource::getLoadPriority() {
    float highestPriority = -FLT_MAX;
    for (QHash<QPointer<QObject>, float>::iterator it = _loadPriorities.begin(); it != _loadPriorities.end(); ) {
        if (it.key().isNull()) {
            it = _loadPriorities.erase(it);
            continue;
        }
        highestPriority = qMax(highestPriority, it.value());
        it++;
    }
    return highestPriority;
}

void Resource::attemptRequest() {
    _startedLoading = true;
    ResourceCache::attemptRequest(this);
}

void Resource::finishedLoading(bool success) {
    if (success) {
        _loaded = true;
    } else {
        _failedToLoad = true;
    }
    _loadPriorities.clear();
}

void Resource::handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (!_reply->isFinished()) {
        return;
    }
    _reply->disconnect(this);
    _reply->deleteLater();
    QNetworkReply* reply = _reply;
    _reply = NULL;
    ResourceCache::requestCompleted();
    
    downloadFinished(reply);
}

void Resource::handleReplyError() {
    QDebug debug = qDebug() << _reply->errorString();
    
    QNetworkReply::NetworkError error = _reply->error();
    _reply->disconnect(this);
    _reply->deleteLater();
    _reply = NULL;
    ResourceCache::requestCompleted();
    
    // retry for certain types of failures
    switch (error) {
        case QNetworkReply::RemoteHostClosedError:
        case QNetworkReply::TimeoutError:
        case QNetworkReply::TemporaryNetworkFailureError:
        case QNetworkReply::ProxyConnectionClosedError:
        case QNetworkReply::ProxyTimeoutError:
        case QNetworkReply::UnknownNetworkError:
        case QNetworkReply::UnknownProxyError:
        case QNetworkReply::UnknownContentError:
        case QNetworkReply::ProtocolFailure: {        
            // retry with increasing delays
            const int MAX_ATTEMPTS = 8;
            const int BASE_DELAY_MS = 1000;
            if (++_attempts < MAX_ATTEMPTS) {
                QTimer::singleShot(BASE_DELAY_MS * (int)pow(2.0, _attempts), this, SLOT(attemptRequest()));
                debug << " -- retrying...";
                return;
            }
            // fall through to final failure
        }    
        default:
            finishedLoading(false);
            break;
    }
}

void Resource::makeRequest() {
    _reply = ResourceCache::getNetworkAccessManager()->get(_request);
    
    connect(_reply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(handleDownloadProgress(qint64,qint64)));
    connect(_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(handleReplyError()));
}

uint qHash(const QPointer<QObject>& value, uint seed) {
    return qHash(value.data(), seed);
}
