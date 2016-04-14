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
    resetResourceCounters();
    
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
    resetResourceCounters();
}

void ResourceCache::addUnusedResource(const QSharedPointer<Resource>& resource) {
    // If it doesn't fit or its size is unknown, remove it from the cache.
    if (resource->getBytes() == 0 || resource->getBytes() > _unusedResourcesMaxSize) {
        resource->setCache(nullptr);

        _totalResourcesSize -= resource->getBytes();
        _resources.remove(resource->getURL());
        resetResourceCounters();

        return;
    }
    reserveUnusedResource(resource->getBytes());
    
    resource->setLRUKey(++_lastLRUKey);
    _unusedResources.insert(resource->getLRUKey(), resource);
    _unusedResourcesSize += resource->getBytes();

    resetResourceCounters();
}

void ResourceCache::removeUnusedResource(const QSharedPointer<Resource>& resource) {
    if (_unusedResources.contains(resource->getLRUKey())) {
        _unusedResources.remove(resource->getLRUKey());
        _unusedResourcesSize -= resource->getBytes();
    }
    resetResourceCounters();
}

void ResourceCache::reserveUnusedResource(qint64 resourceSize) {
    while (!_unusedResources.empty() &&
           _unusedResourcesSize + resourceSize > _unusedResourcesMaxSize) {
        // unload the oldest resource
        QMap<int, QSharedPointer<Resource> >::iterator it = _unusedResources.begin();
        
        it.value()->setCache(nullptr);
        auto size = it.value()->getBytes();

        _totalResourcesSize -= size;
        _resources.remove(it.value()->getURL());

        _unusedResourcesSize -= size;
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

void ResourceCache::resetResourceCounters() {
    _numTotalResources = _resources.size();
    _numUnusedResources = _unusedResources.size();
    emit dirty();
}

void ResourceCache::updateTotalSize(const qint64& oldSize, const qint64& newSize) {
    _totalResourcesSize += (newSize - oldSize);
    emit dirty();
}

void ResourceCacheSharedItems::appendActiveRequest(Resource* resource) {
    Lock lock(_mutex);
    _loadingRequests.append(resource);
}

void ResourceCacheSharedItems::appendPendingRequest(Resource* resource) {
    Lock lock(_mutex);
    _pendingRequests.append(resource);
}

QList<QPointer<Resource>> ResourceCacheSharedItems::getPendingRequests() const {
    Lock lock(_mutex);
    return _pendingRequests;
}

uint32_t ResourceCacheSharedItems::getPendingRequestsCount() const {
    Lock lock(_mutex);
    return _pendingRequests.size();
}

QList<Resource*> ResourceCacheSharedItems::getLoadingRequests() const {
    Lock lock(_mutex);
    return _loadingRequests;
}

void ResourceCacheSharedItems::removeRequest(Resource* resource) {
    Lock lock(_mutex);
    _loadingRequests.removeOne(resource);
}

Resource* ResourceCacheSharedItems::getHighestPendingRequest() {
    Lock lock(_mutex);
    // look for the highest priority pending request
    int highestIndex = -1;
    float highestPriority = -FLT_MAX;
    for (int i = 0; i < _pendingRequests.size();) {
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
        return _pendingRequests.takeAt(highestIndex);
    }
    return nullptr;
}

bool ResourceCache::attemptRequest(Resource* resource) {
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

void ResourceCache::requestCompleted(Resource* resource) {
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
    
    // start loading immediately unless instructed otherwise
    if (!(_startedLoading || delayLoad)) {    
        QTimer::singleShot(0, this, &Resource::ensureLoading);
    }
}

Resource::~Resource() {
    if (_request) {
        _request->disconnect(this);
        _request->deleteLater();
        _request = nullptr;
        ResourceCache::requestCompleted(this);
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
        ResourceCache::requestCompleted(this);
    }
    
    init();
    ensureLoading();
    emit onRefresh();
}

void Resource::allReferencesCleared() {
    if (_cache && isCacheable()) {
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

void Resource::attemptRequest() {
    _startedLoading = true;
    ResourceCache::attemptRequest(this);
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
    _cache->_resources.insert(_url, _self);
}


void Resource::makeRequest() {
    Q_ASSERT(!_request);

    _request = ResourceManager::createResourceRequest(this, _activeUrl);

    if (!_request) {
        qCDebug(networking).noquote() << "Failed to get request for" << _url.toDisplayString();
        ResourceCache::requestCompleted(this);
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
    
    ResourceCache::requestCompleted(this);
    
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
                const int MAX_ATTEMPTS = 8;
                const int BASE_DELAY_MS = 1000;
                if (_attempts++ < MAX_ATTEMPTS) {
                    auto waitTime = BASE_DELAY_MS * (int)pow(2.0, _attempts);
                    qCDebug(networking).nospace() << "Retrying to load the asset in " << waitTime
                                       << "ms, attempt " << _attempts << " of " << MAX_ATTEMPTS;
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
