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

#include <SharedUtil.h>
#include <assert.h>

#include "NetworkAccessManager.h"
#include "NetworkLogging.h"
#include "NodeList.h"

#include "ResourceCache.h"

#define clamp(x, min, max) (((x) < (min)) ? (min) :\
                           (((x) > (max)) ? (max) :\
                                            (x)))

void ResourceCacheSharedItems::appendActiveRequest(QWeakPointer<Resource> resource) {
    Lock lock(_mutex);
    _loadingRequests.append(resource);
}

void ResourceCacheSharedItems::appendPendingRequest(QWeakPointer<Resource> resource) {
    Lock lock(_mutex);
    _pendingRequests.append(resource);
}

QList<QSharedPointer<Resource>> ResourceCacheSharedItems::getPendingRequests() {
    QList<QSharedPointer<Resource>> result;

    {
        Lock lock(_mutex);
        foreach(QSharedPointer<Resource> resource, _pendingRequests) {
            if (resource) {
                result.append(resource);
            }
        }
    }
    return result;
}

uint32_t ResourceCacheSharedItems::getPendingRequestsCount() const {
    Lock lock(_mutex);
    return _pendingRequests.size();
}

QList<QSharedPointer<Resource>> ResourceCacheSharedItems::getLoadingRequests() {
    QList<QSharedPointer<Resource>> result;

    {
        Lock lock(_mutex);
        foreach(QSharedPointer<Resource> resource, _loadingRequests) {
            if (resource) {
                result.append(resource);
            }
        }
    }
    return result;
}

void ResourceCacheSharedItems::removeRequest(QWeakPointer<Resource> resource) {
    Lock lock(_mutex);
    // resource can only be removed if it still has a ref-count, as
    // QWeakPointer has no operator== implementation for two weak ptrs, so
    // manually loop in case resource has been freed.
    for (int i = 0; i < _loadingRequests.size();) {
        auto request = _loadingRequests.at(i);
        // Clear our resource and any freed resources
        if (!request || request.data() == resource.data()) {
            _loadingRequests.removeAt(i);
            continue;
        }
        i++;
    }
}

QSharedPointer<Resource> ResourceCacheSharedItems::getHighestPendingRequest() {
    Lock lock(_mutex);
    // look for the highest priority pending request
    int highestIndex = -1;
    float highestPriority = -FLT_MAX;
    QSharedPointer<Resource> highestResource;

    for (int i = 0; i < _pendingRequests.size();) {
        // Clear any freed resources
        auto resource = _pendingRequests.at(i).lock();
        if (!resource) {
            _pendingRequests.removeAt(i);
            continue;
        }

        // Check load priority
        float priority = resource->getLoadPriority();
        if (priority >= highestPriority) {
            highestPriority = priority;
            highestIndex = i;
            highestResource = resource;
        }
        i++;
    }

    if (highestIndex >= 0) {
        _pendingRequests.takeAt(highestIndex);
    }

    return highestResource;
}

ResourceCache::ResourceCache(QObject* parent) : QObject(parent) {
    auto nodeList = DependencyManager::get<NodeList>();
    if (nodeList) {
        auto& domainHandler = nodeList->getDomainHandler();
        connect(&domainHandler, &DomainHandler::disconnectedFromDomain,
            this, &ResourceCache::clearATPAssets, Qt::DirectConnection);
    }
}

ResourceCache::~ResourceCache() {
    clearUnusedResource();
}

void ResourceCache::clearATPAssets() {
    {
        QWriteLocker locker(&_resourcesLock);
        for (auto& url : _resources.keys()) {
            // If this is an ATP resource
            if (url.scheme() == URL_SCHEME_ATP) {

                // Remove it from the resource hash
                auto resource = _resources.take(url);
                if (auto strongRef = resource.lock()) {
                    // Make sure the resource won't reinsert itself
                    strongRef->setCache(nullptr);
                }
            }
        }
    }
    {
        QWriteLocker locker(&_unusedResourcesLock);
        for (auto& resource : _unusedResources.values()) {
            if (resource->getURL().scheme() == URL_SCHEME_ATP) {
                _unusedResources.remove(resource->getLRUKey());
            }
        }
    }
    {
        QWriteLocker locker(&_resourcesToBeGottenLock);
        auto it = _resourcesToBeGotten.begin();
        while (it != _resourcesToBeGotten.end()) {
            if (it->scheme() == URL_SCHEME_ATP) {
                it = _resourcesToBeGotten.erase(it);
            } else {
                ++it;
            }
        }
    }


}

void ResourceCache::refreshAll() {
    // Clear all unused resources so we don't have to reload them
    clearUnusedResource();
    resetResourceCounters();

    _resourcesLock.lockForRead();
    auto resourcesCopy = _resources;
    _resourcesLock.unlock();

    // Refresh all remaining resources in use
    foreach (QSharedPointer<Resource> resource, resourcesCopy) {
        if (resource) {
            resource->refresh();
        }
    }
}

void ResourceCache::refresh(const QUrl& url) {
    QSharedPointer<Resource> resource;
    {
        QReadLocker locker(&_resourcesLock);
        resource = _resources.value(url).lock();
    }

    if (resource) {
        resource->refresh();
    } else {
        removeResource(url);
        resetResourceCounters();
    }
}

QVariantList ResourceCache::getResourceList() {
    QVariantList list;
    if (QThread::currentThread() != thread()) {
        // NOTE: invokeMethod does not allow a const QObject*
        QMetaObject::invokeMethod(this, "getResourceList", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QVariantList, list));
    } else {
        auto resources = _resources.uniqueKeys();
        list.reserve(resources.size());
        for (auto& resource : resources) {
            list << resource;
        }
    }

    return list;
}

void ResourceCache::setRequestLimit(int limit) {
    _requestLimit = limit;

    // Now go fill any new request spots
    while (attemptHighestPriorityRequest()) {
        // just keep looping until we reach the new limit or no more pending requests
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
    QSharedPointer<Resource> resource;
    {
        QReadLocker locker(&_resourcesLock);
        resource = _resources.value(url).lock();
    }
    if (resource) {
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
    {
        QWriteLocker locker(&_resourcesLock);
        _resources.insert(url, resource);
    }
    removeUnusedResource(resource);
    resource->ensureLoading();

    return resource;
}

void ResourceCache::setUnusedResourceCacheSize(qint64 unusedResourcesMaxSize) {
    _unusedResourcesMaxSize = clamp(unusedResourcesMaxSize, MIN_UNUSED_MAX_SIZE, MAX_UNUSED_MAX_SIZE);
    reserveUnusedResource(0);
    resetResourceCounters();
}

void ResourceCache::addUnusedResource(const QSharedPointer<Resource>& resource) {
    // If it doesn't fit or its size is unknown, remove it from the cache.
    if (resource->getBytes() == 0 || resource->getBytes() > _unusedResourcesMaxSize) {
        resource->setCache(nullptr);
        removeResource(resource->getURL(), resource->getBytes());
        resetResourceCounters();
        return;
    }
    reserveUnusedResource(resource->getBytes());
    
    resource->setLRUKey(++_lastLRUKey);
    _unusedResourcesSize += resource->getBytes();

    resetResourceCounters();

    QWriteLocker locker(&_unusedResourcesLock);
    _unusedResources.insert(resource->getLRUKey(), resource);
}

void ResourceCache::removeUnusedResource(const QSharedPointer<Resource>& resource) {
    QWriteLocker locker(&_unusedResourcesLock);
    if (_unusedResources.contains(resource->getLRUKey())) {
        _unusedResources.remove(resource->getLRUKey());
        _unusedResourcesSize -= resource->getBytes();
    }
    resetResourceCounters();
}

void ResourceCache::reserveUnusedResource(qint64 resourceSize) {
    QWriteLocker locker(&_unusedResourcesLock);
    while (!_unusedResources.empty() &&
           _unusedResourcesSize + resourceSize > _unusedResourcesMaxSize) {
        // unload the oldest resource
        QMap<int, QSharedPointer<Resource> >::iterator it = _unusedResources.begin();
        
        it.value()->setCache(nullptr);
        auto size = it.value()->getBytes();

        removeResource(it.value()->getURL(), size);

        _unusedResourcesSize -= size;
        _unusedResources.erase(it);
    }
}

void ResourceCache::clearUnusedResource() {
    // the unused resources may themselves reference resources that will be added to the unused
    // list on destruction, so keep clearing until there are no references left
    QWriteLocker locker(&_unusedResourcesLock);
    while (!_unusedResources.isEmpty()) {
        foreach (const QSharedPointer<Resource>& resource, _unusedResources) {
            resource->setCache(nullptr);
        }
        _unusedResources.clear();
    }
}

void ResourceCache::resetResourceCounters() {
    _numTotalResources = _resources.size();
    _numUnusedResources = _unusedResources.size();
    emit dirty();
}

void ResourceCache::removeResource(const QUrl& url, qint64 size) {
    QWriteLocker locker(&_resourcesLock);
    _resources.remove(url);
    _totalResourcesSize -= size;
}

void ResourceCache::updateTotalSize(const qint64& oldSize, const qint64& newSize) {
    _totalResourcesSize += (newSize - oldSize);
    emit dirty();
}
 
QList<QSharedPointer<Resource>> ResourceCache::getLoadingRequests() {
    return DependencyManager::get<ResourceCacheSharedItems>()->getLoadingRequests();
}

int ResourceCache::getPendingRequestCount() {
    return DependencyManager::get<ResourceCacheSharedItems>()->getPendingRequestsCount();
}

bool ResourceCache::attemptRequest(QSharedPointer<Resource> resource) {
    auto sharedItems = DependencyManager::get<ResourceCacheSharedItems>();

    if (_requestsActive >= _requestLimit) {
        // wait until a slot becomes available
        sharedItems->appendPendingRequest(resource);
        return false;
    }
    
    ++_requestsActive;
    sharedItems->appendActiveRequest(resource);
    resource->makeRequest();
    return true;
}

void ResourceCache::requestCompleted(QWeakPointer<Resource> resource) {
    auto sharedItems = DependencyManager::get<ResourceCacheSharedItems>();
    sharedItems->removeRequest(resource);
    --_requestsActive;

    attemptHighestPriorityRequest();
}

bool ResourceCache::attemptHighestPriorityRequest() {
    auto sharedItems = DependencyManager::get<ResourceCacheSharedItems>();
    auto resource = sharedItems->getHighestPendingRequest();
    return (resource && attemptRequest(resource));
}

const int DEFAULT_REQUEST_LIMIT = 10;
int ResourceCache::_requestLimit = DEFAULT_REQUEST_LIMIT;
int ResourceCache::_requestsActive = 0;

Resource::Resource(const QUrl& url, bool delayLoad) :
    _url(url),
    _activeUrl(url),
    _request(nullptr) {
    
    init();
}

Resource::~Resource() {
    if (_request) {
        _request->disconnect(this);
        _request->deleteLater();
        _request = nullptr;
        ResourceCache::requestCompleted(_self);
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
    if (_request && !(_loaded || _failedToLoad)) {
        return;
    }
    if (_request) {
        _request->disconnect(this);
        _request->deleteLater();
        _request = nullptr;
        ResourceCache::requestCompleted(_self);
    }
    
    init();
    ensureLoading();
    emit onRefresh();
}

void Resource::allReferencesCleared() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "allReferencesCleared");
        return;
    }

    if (_cache && isCacheable()) {
        // create and reinsert new shared pointer 
        QSharedPointer<Resource> self(this, &Resource::allReferencesCleared);
        setSelf(self);
        reinsert();

        // add to the unused list
        _cache->addUnusedResource(self);
    } else {
        if (_cache) {
            // remove from the cache
            _cache->removeResource(getURL(), getBytes());
            _cache->resetResourceCounters();
        }

        deleteLater();
    }
}

void Resource::init() {
    _startedLoading = false;
    _failedToLoad = false;
    _loaded = false;
    _attempts = 0;
    _activeUrl = _url;
    
    if (_url.isEmpty()) {
        _startedLoading = _loaded = true;
        
    } else if (!(_url.isValid())) {
        _startedLoading = _failedToLoad = true;
    }
}

const int MAX_ATTEMPTS = 8;

void Resource::attemptRequest() {
    _startedLoading = true;

    if (_attempts > 0) {
        qCDebug(networking).noquote() << "Server unavailable for" << _url
            << "- retrying asset load - attempt" << _attempts << " of " << MAX_ATTEMPTS;
    }

    ResourceCache::attemptRequest(_self);
}

void Resource::finishedLoading(bool success) {
    if (success) {
        qCDebug(networking).noquote() << "Finished loading:" << _url.toDisplayString();
        _loaded = true;
    } else {
        qCDebug(networking).noquote() << "Failed to load:" << _url.toDisplayString();
        _failedToLoad = true;
    }
    _loadPriorities.clear();
    emit finished(success);
}

void Resource::setSize(const qint64& bytes) {
    QMetaObject::invokeMethod(_cache.data(), "updateTotalSize", Q_ARG(qint64, _bytes), Q_ARG(qint64, bytes));
    _bytes = bytes;
}

void Resource::reinsert() {
    QWriteLocker locker(&_cache->_resourcesLock);
    _cache->_resources.insert(_url, _self);
}


void Resource::makeRequest() {
    if (_request) {
        _request->disconnect();
        _request->deleteLater();
    }

    _request = ResourceManager::createResourceRequest(this, _activeUrl);

    if (!_request) {
        qCDebug(networking).noquote() << "Failed to get request for" << _url.toDisplayString();
        ResourceCache::requestCompleted(_self);
        finishedLoading(false);
        return;
    }
    
    qCDebug(networking).noquote() << "Starting request for:" << _url.toDisplayString();

    connect(_request, &ResourceRequest::progress, this, &Resource::handleDownloadProgress);
    connect(_request, &ResourceRequest::finished, this, &Resource::handleReplyFinished);

    _bytesReceived = _bytesTotal = _bytes = 0;

    _request->send();
}

void Resource::handleDownloadProgress(uint64_t bytesReceived, uint64_t bytesTotal) {
    _bytesReceived = bytesReceived;
    _bytesTotal = bytesTotal;
}

void Resource::handleReplyFinished() {
    Q_ASSERT_X(_request, "Resource::handleReplyFinished", "Request should not be null while in handleReplyFinished");

    setSize(_bytesTotal);

    if (!_request || _request != sender()) {
        // This can happen in the edge case that a request is timed out, but a `finished` signal is emitted before it is deleted.
        qWarning(networking) << "Received signal Resource::handleReplyFinished from ResourceRequest that is not the current"
            << " request: " << sender() << ", " << _request;
        return;
    }
    
    ResourceCache::requestCompleted(_self);
    
    auto result = _request->getResult();
    if (result == ResourceRequest::Success) {
        auto extraInfo = _url == _activeUrl ? "" : QString(", %1").arg(_activeUrl.toDisplayString());
        qCDebug(networking).noquote() << QString("Request finished for %1%2").arg(_url.toDisplayString(), extraInfo);
        
        auto data = _request->getData();
        emit loaded(data);
        downloadFinished(data);
    } else {
        switch (result) {
            case ResourceRequest::Result::Timeout: {
                qCDebug(networking) << "Timed out loading" << _url << "received" << _bytesReceived << "total" << _bytesTotal;
                // Fall through to other cases
            }
            case ResourceRequest::Result::ServerUnavailable: {
                // retry with increasing delays
                const int BASE_DELAY_MS = 1000;
                if (_attempts++ < MAX_ATTEMPTS) {
                    auto waitTime = BASE_DELAY_MS * (int)pow(2.0, _attempts);

                    qCDebug(networking).noquote() << "Server unavailable for" << _url << "- may retry in" << waitTime << "ms"
                        << "if resource is still needed";

                    QTimer::singleShot(waitTime, this, &Resource::attemptRequest);
                    break;
                }
                // fall through to final failure
            }
            default: {
                qCDebug(networking) << "Error loading " << _url;
                auto error = (result == ResourceRequest::Timeout) ? QNetworkReply::TimeoutError
                                                                  : QNetworkReply::UnknownNetworkError;
                emit failed(error);
                finishedLoading(false);
                break;
            }
        }
    }
    
    _request->disconnect(this);
    _request->deleteLater();
    _request = nullptr;
}

uint qHash(const QPointer<QObject>& value, uint seed) {
    return qHash(value.data(), seed);
}
