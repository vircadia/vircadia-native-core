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

#include <QDebug>
#include <QNetworkDiskCache>
#include <QThread>
#include <QTimer>

#include <SharedUtil.h>
#include <assert.h>

#include "NetworkAccessManager.h"
#include "NetworkLogging.h"

#include "ResourceCache.h"

#define clamp(x, min, max) (((x) < (min)) ? (min) :\
                           (((x) > (max)) ? (max) :\
                                            (x)))

ResourceCache::ResourceCache(QObject* parent) :
    QObject(parent) {    
}

ResourceCache::~ResourceCache() {
    clearUnusedResource();
}

void ResourceCache::refreshAll() {
    // Clear all unused resources so we don't have to reload them
    clearUnusedResource();
    
    // Refresh all remaining resources in use
    foreach (auto resource, _resources) {
        if (!resource.isNull()) {
            resource.data()->refresh();
        }
    }
}

void ResourceCache::refresh(const QUrl& url) {
    QSharedPointer<Resource> resource = _resources.value(url);
    if (!resource.isNull()) {
        resource->refresh();
    } else {
        _resources.remove(url);
    }
}

void ResourceCache::getResourceAsynchronously(const QUrl& url) {
    qCDebug(networking) << "ResourceCache::getResourceAsynchronously" << url.toString();
    _resourcesToBeGottenLock.lockForWrite();
    _resourcesToBeGotten.enqueue(QUrl(url));
    _resourcesToBeGottenLock.unlock();
}

void ResourceCache::checkAsynchronousGets() {
    assert(QThread::currentThread() == thread());
    if (!_resourcesToBeGotten.isEmpty()) {
        _resourcesToBeGottenLock.lockForWrite();
        QUrl url = _resourcesToBeGotten.dequeue();
        _resourcesToBeGottenLock.unlock();
        getResource(url);
    }
}

QSharedPointer<Resource> ResourceCache::getResource(const QUrl& url, const QUrl& fallback,
                                                    bool delayLoad, void* extra) {
    QSharedPointer<Resource> resource = _resources.value(url);
    if (!resource.isNull()) {
        removeUnusedResource(resource);
        return resource;
    }

    if (QThread::currentThread() != thread()) {
        assert(delayLoad);
        getResourceAsynchronously(url);
        return QSharedPointer<Resource>();
    }

    if (!url.isValid() && !url.isEmpty() && fallback.isValid()) {
        return getResource(fallback, QUrl(), delayLoad);
    }

    resource = createResource(url, fallback.isValid() ?
                              getResource(fallback, QUrl(), true) : QSharedPointer<Resource>(), delayLoad, extra);
    resource->setSelf(resource);
    resource->setCache(this);
    _resources.insert(url, resource);
    removeUnusedResource(resource);
    resource->ensureLoading();

    return resource;
}

void ResourceCache::setUnusedResourceCacheSize(qint64 unusedResourcesMaxSize) {
    _unusedResourcesMaxSize = clamp(unusedResourcesMaxSize, MIN_UNUSED_MAX_SIZE, MAX_UNUSED_MAX_SIZE);
    reserveUnusedResource(0);
}

void ResourceCache::addUnusedResource(const QSharedPointer<Resource>& resource) {
    if (resource->getBytesTotal() > _unusedResourcesMaxSize) {
        // If it doesn't fit anyway, let's leave whatever is already in the cache.
        resource->setCache(nullptr);
        return;
    }
    reserveUnusedResource(resource->getBytesTotal());
    
    resource->setLRUKey(++_lastLRUKey);
    _unusedResources.insert(resource->getLRUKey(), resource);
    _unusedResourcesSize += resource->getBytesTotal();
}

void ResourceCache::removeUnusedResource(const QSharedPointer<Resource>& resource) {
    if (_unusedResources.contains(resource->getLRUKey())) {
        _unusedResources.remove(resource->getLRUKey());
        _unusedResourcesSize -= resource->getBytesTotal();
    }
}

void ResourceCache::reserveUnusedResource(qint64 resourceSize) {
    while (!_unusedResources.empty() &&
           _unusedResourcesSize + resourceSize > _unusedResourcesMaxSize) {
        // unload the oldest resource
        QMap<int, QSharedPointer<Resource> >::iterator it = _unusedResources.begin();
        
        _unusedResourcesSize -= it.value()->getBytesTotal();
        it.value()->setCache(nullptr);
        _unusedResources.erase(it);
    }
}

void ResourceCache::clearUnusedResource() {
    // the unused resources may themselves reference resources that will be added to the unused
    // list on destruction, so keep clearing until there are no references left
    while (!_unusedResources.isEmpty()) {
        foreach (const QSharedPointer<Resource>& resource, _unusedResources) {
            resource->setCache(nullptr);
        }
        _unusedResources.clear();
    }
}

void ResourceCache::attemptRequest(Resource* resource) {
    auto sharedItems = DependencyManager::get<ResourceCacheSharedItems>();
    if (_requestLimit <= 0) {
        // wait until a slot becomes available
        sharedItems->_pendingRequests.append(resource);
        return;
    }
    _requestLimit--;
    sharedItems->_loadingRequests.append(resource);
    resource->makeRequest();
}

void ResourceCache::requestCompleted(Resource* resource) {
    
    auto sharedItems = DependencyManager::get<ResourceCacheSharedItems>();
    sharedItems->_loadingRequests.removeOne(resource);
    _requestLimit++;
    
    // look for the highest priority pending request
    int highestIndex = -1;
    float highestPriority = -FLT_MAX;
    for (int i = 0; i < sharedItems->_pendingRequests.size(); ) {
        Resource* resource = sharedItems->_pendingRequests.at(i).data();
        if (!resource) {
            sharedItems->_pendingRequests.removeAt(i);
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
        attemptRequest(sharedItems->_pendingRequests.takeAt(highestIndex));
    }
}

const int DEFAULT_REQUEST_LIMIT = 10;
int ResourceCache::_requestLimit = DEFAULT_REQUEST_LIMIT;

Resource::Resource(const QUrl& url, bool delayLoad) :
    _url(url),
    _request(url) {
    
    init();
    
    _request.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
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
        _reply = nullptr;
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
    if (_reply && !(_loaded || _failedToLoad)) {
        return;
    }
    if (_reply) {
        ResourceCache::requestCompleted(this);
        _reply->disconnect(this);
        _replyTimer->disconnect(this);
        _reply->deleteLater();
        _reply = nullptr;
        _replyTimer->deleteLater();
        _replyTimer = nullptr;
    }
    
    init();
    _request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    ensureLoading();
    emit onRefresh();
}

void Resource::allReferencesCleared() {
    if (_cache) {
        if (QThread::currentThread() != thread()) {
            QMetaObject::invokeMethod(this, "allReferencesCleared");
            return;
        }
        
        // create and reinsert new shared pointer 
        QSharedPointer<Resource> self(this, &Resource::allReferencesCleared);
        setSelf(self);
        reinsert();
        
        // add to the unused list
        _cache->addUnusedResource(self);
        
    } else {
        delete this;
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
    } else {
        _failedToLoad = true;
    }
    _loadPriorities.clear();
}

void Resource::reinsert() {
    _cache->_resources.insert(_url, _self);
}

static const int REPLY_TIMEOUT_MS = 5000;
void Resource::handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    _bytesReceived = bytesReceived;
    _bytesTotal = bytesTotal;
    _replyTimer->start(REPLY_TIMEOUT_MS);
}

void Resource::handleReplyError() {
    handleReplyErrorInternal(_reply->error());
}

void Resource::handleReplyTimeout() {
    handleReplyErrorInternal(QNetworkReply::TimeoutError);
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

void Resource::handleReplyErrorInternal(QNetworkReply::NetworkError error) {

    _reply->disconnect(this);
    _replyTimer->disconnect(this);
    _reply->deleteLater();
    _reply = nullptr;
    _replyTimer->deleteLater();
    _replyTimer = nullptr;
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
                qCWarning(networking) << "error downloading url =" << _url.toDisplayString() << ", error =" << error << ", retrying (" << _attempts << "/" << MAX_ATTEMPTS << ")";
                return;
            }
            // fall through to final failure
        }
        default:
            qCCritical(networking) << "error downloading, url =" << _url.toDisplayString() << ", error =" << error;
            emit failed(error);
            finishedLoading(false);
            break;
    }
}

void Resource::handleReplyFinished() {

    bool fromCache = _reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool();
    qCDebug(networking) << "success downloading url =" << _url.toDisplayString() << (fromCache ? "from cache" : "");

    _reply->disconnect(this);
    _replyTimer->disconnect(this);
    QNetworkReply* reply = _reply;
    _reply = nullptr;
    _replyTimer->deleteLater();
    _replyTimer = nullptr;
    ResourceCache::requestCompleted(this);

    finishedLoading(true);
    emit loaded(*reply);
    downloadFinished(reply);
}


void Resource::downloadFinished(QNetworkReply* reply) {
    ;
}

uint qHash(const QPointer<QObject>& value, uint seed) {
    return qHash(value.data(), seed);
}
