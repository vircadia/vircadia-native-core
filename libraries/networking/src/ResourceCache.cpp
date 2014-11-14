//
//  ResourceCache.cpp
//  libraries/shared/src
//
//  Created by Andrzej Kapolka on 2/27/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cfloat>
#include <cmath>

#include <QThread>
#include <QTimer>
#include <QtDebug>

#include "NetworkAccessManager.h"

#include "ResourceCache.h"

ResourceCache::ResourceCache(QObject* parent) :
    QObject(parent),
    _lastLRUKey(0)
{
    
}

ResourceCache::~ResourceCache() {
    // make sure our unused resources know we're out of commission
    foreach (const QSharedPointer<Resource>& resource, _unusedResources) {
        resource->setCache(NULL);
    }
}

void ResourceCache::refresh(const QUrl& url) {
    QSharedPointer<Resource> resource = _resources.value(url);
    if (!resource.isNull()) {
        resource->refresh();
    }
}

QSharedPointer<Resource> ResourceCache::getResource(const QUrl& url, const QUrl& fallback, bool delayLoad, void* extra) {

    if (QThread::currentThread() != thread()) {
        QSharedPointer<Resource> result;
        QMetaObject::invokeMethod(this, "getResource", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(QSharedPointer<Resource>, result), Q_ARG(const QUrl&, url), Q_ARG(const QUrl&, fallback),
                                  Q_ARG(bool, delayLoad), Q_ARG(void*, extra));
        return result;
    }

    if (!url.isValid() && !url.isEmpty() && fallback.isValid()) {
        return getResource(fallback, QUrl(), delayLoad);
    }
    QSharedPointer<Resource> resource = _resources.value(url);
    if (resource.isNull()) {
        resource = createResource(url, fallback.isValid() ?
            getResource(fallback, QUrl(), true) : QSharedPointer<Resource>(), delayLoad, extra);
        resource->setSelf(resource);
        resource->setCache(this);
        _resources.insert(url, resource);
        
    } else {
        _unusedResources.remove(resource->getLRUKey());
    }
    return resource;
}

void ResourceCache::addUnusedResource(const QSharedPointer<Resource>& resource) {
    const int RETAINED_RESOURCE_COUNT = 50;
    if (_unusedResources.size() > RETAINED_RESOURCE_COUNT) {
        // unload the oldest resource
        QMap<int, QSharedPointer<Resource> >::iterator it = _unusedResources.begin();
        it.value()->setCache(NULL);
        _unusedResources.erase(it);
    }
    resource->setLRUKey(++_lastLRUKey);
    _unusedResources.insert(resource->getLRUKey(), resource);
}

void ResourceCache::attemptRequest(Resource* resource) {
    if (_requestLimit <= 0) {
        // wait until a slot becomes available
        _pendingRequests.append(resource);
        return;
    }
    _requestLimit--;
    _loadingRequests.append(resource);
    resource->makeRequest();
}

void ResourceCache::requestCompleted(Resource* resource) {
    _loadingRequests.removeOne(resource);
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

const int DEFAULT_REQUEST_LIMIT = 10;
int ResourceCache::_requestLimit = DEFAULT_REQUEST_LIMIT;

QList<QPointer<Resource> > ResourceCache::_pendingRequests;
QList<Resource*> ResourceCache::_loadingRequests;

Resource::Resource(const QUrl& url, bool delayLoad) :
    _url(url),
    _request(url),
    _lruKey(0),
    _reply(NULL) {
    
    init();
    
    _request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    
    // start loading immediately unless instructed otherwise
    if (!(_startedLoading || delayLoad)) {    
        attemptRequest();
    }
}

Resource::~Resource() {
    if (_reply) {
        ResourceCache::requestCompleted(this);
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

void Resource::refresh() {
    if (_reply == NULL && !(_loaded || _failedToLoad)) {
        return;
    }
    if (_reply) {
        ResourceCache::requestCompleted(this);
        delete _reply;
        _reply = NULL;
    }
    init();
    _request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    if (!_startedLoading) {
        attemptRequest();
    }
}

void Resource::allReferencesCleared() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "allReferencesCleared");
        return;
    }
    if (_cache) {
        // create and reinsert new shared pointer 
        QSharedPointer<Resource> self(this, &Resource::allReferencesCleared);
        setSelf(self);
        reinsert();
        
        // add to the unused list
        _cache->addUnusedResource(self);
        
    } else {
#ifndef WIN32
        // Note to Andrzej this causes a consistent crash on windows/vs2013
        // patching here as a very temporary workaround.  --craig
        delete this;
#endif // !WIN32
    }
}

void Resource::init() {
    _startedLoading = false;
    _failedToLoad = false;
    _loaded = false;
    _attempts = 0;
    
    if (_url.isEmpty()) {
        _startedLoading = _loaded = true;
        
    } else if (!(_url.isValid())) {
        _startedLoading = _failedToLoad = true;
    }
}

void Resource::attemptRequest() {
    _startedLoading = true;
    ResourceCache::attemptRequest(this);
}

void Resource::finishedLoading(bool success) {
    if (success) {
        _loaded = true;
        emit loaded();
    } else {
        _failedToLoad = true;
    }
    _loadPriorities.clear();
}

void Resource::reinsert() {
    _cache->_resources.insert(_url, _self);
}

const int REPLY_TIMEOUT_MS = 5000;

void Resource::handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (!_reply->isFinished()) {
        _bytesReceived = bytesReceived;
        _bytesTotal = bytesTotal;
        _replyTimer->start(REPLY_TIMEOUT_MS);
        return;
    }
    _reply->disconnect(this);
    QNetworkReply* reply = _reply;
    _reply = NULL;
    _replyTimer->disconnect(this);
    _replyTimer->deleteLater();
    _replyTimer = NULL;
    ResourceCache::requestCompleted(this);
    
    downloadFinished(reply);
}

void Resource::handleReplyError() {
    handleReplyError(_reply->error(), qDebug() << _reply->errorString());
}

void Resource::handleReplyTimeout() {
    handleReplyError(QNetworkReply::TimeoutError, qDebug() << "Timed out loading" << _reply->url() <<
        "received" << _bytesReceived << "total" << _bytesTotal);
}

void Resource::makeRequest() {
    _reply = NetworkAccessManager::getInstance().get(_request);
    
    connect(_reply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(handleDownloadProgress(qint64,qint64)));
    connect(_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(handleReplyError()));
    connect(_reply, SIGNAL(finished()), SLOT(handleReplyFinished()));

    _replyTimer = new QTimer(this);
    connect(_replyTimer, SIGNAL(timeout()), SLOT(handleReplyTimeout()));
    _replyTimer->setSingleShot(true);
    _replyTimer->start(REPLY_TIMEOUT_MS);
    _bytesReceived = _bytesTotal = 0;
}

void Resource::handleReplyError(QNetworkReply::NetworkError error, QDebug debug) {
    _reply->disconnect(this);
    _reply->deleteLater();
    _reply = NULL;
    _replyTimer->disconnect(this);
    _replyTimer->deleteLater();
    _replyTimer = NULL;
    ResourceCache::requestCompleted(this);
    
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
                debug << "-- retrying...";
                return;
            }
            // fall through to final failure
        }    
        default:
            finishedLoading(false);
            break;
    }
}

void Resource::handleReplyFinished() {
    qDebug() << "Got finished without download progress/error?" << _url;
    handleDownloadProgress(0, 0);
}

uint qHash(const QPointer<QObject>& value, uint seed) {
    return qHash(value.data(), seed);
}
