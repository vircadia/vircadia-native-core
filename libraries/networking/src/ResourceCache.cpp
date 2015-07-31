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
        emit loaded();
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
    if (!_reply->isFinished()) {
        _bytesReceived = bytesReceived;
        _bytesTotal = bytesTotal;
        _replyTimer->start(REPLY_TIMEOUT_MS);
        return;
    }
    _reply->disconnect(this);
    _replyTimer->disconnect(this);
    QNetworkReply* reply = _reply;
    _reply = nullptr;
    _replyTimer->deleteLater();
    _replyTimer = nullptr;
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

void Resource::maybeRefresh() {
    if (Q_LIKELY(NetworkAccessManager::getInstance().cache())) {
        QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
        QVariant variant = reply->header(QNetworkRequest::LastModifiedHeader);
        QNetworkCacheMetaData metaData = NetworkAccessManager::getInstance().cache()->metaData(_url);
        if (variant.isValid() && variant.canConvert<QDateTime>() && metaData.isValid()) {
            QDateTime lastModified = variant.value<QDateTime>();
            QDateTime lastModifiedOld = metaData.lastModified();
            if (lastModified.isValid() && lastModifiedOld.isValid() &&
                lastModifiedOld >= lastModified) { // With >=, cache won't thrash in eventually-consistent cdn.
                qCDebug(networking) << "Using cached version of" << _url.fileName();
                // We don't need to update, return
                return;
            }
        } else if (!variant.isValid() || !variant.canConvert<QDateTime>() ||
                   !variant.value<QDateTime>().isValid() || variant.value<QDateTime>().isNull()) {
            qCDebug(networking) << "Cannot determine when" << _url.fileName() << "was modified last, cached version might be outdated";
            return;
        }
        qCDebug(networking) << "Loaded" << _url.fileName() << "from the disk cache but the network version is newer, refreshing.";
        refresh();
    }
}

void Resource::makeRequest() {
    _reply = NetworkAccessManager::getInstance().get(_request);
    
    connect(_reply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(handleDownloadProgress(qint64,qint64)));
    connect(_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(handleReplyError()));
    connect(_reply, SIGNAL(finished()), SLOT(handleReplyFinished()));
    
    if (_reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool()) {
        // If the file as been updated since it was cached, refresh it
        QNetworkRequest request(_request);
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
        QNetworkReply* reply = NetworkAccessManager::getInstance().head(request);
        connect(reply, &QNetworkReply::finished, this, &Resource::maybeRefresh);
    } else {
        if (Q_LIKELY(NetworkAccessManager::getInstance().cache())) {
            QNetworkCacheMetaData metaData = NetworkAccessManager::getInstance().cache()->metaData(_url);
            bool needUpdate = false;
            if (metaData.expirationDate().isNull() || metaData.expirationDate() <= QDateTime::currentDateTime()) {
                // If the expiration date is NULL or in the past,
                // put one far enough away that it won't be an issue.
                metaData.setExpirationDate(QDateTime::currentDateTime().addYears(100));
                needUpdate = true;
            }
            if (metaData.lastModified().isNull()) {
                // If the lastModified date is NULL, set it to now.
                metaData.setLastModified(QDateTime::currentDateTime());
                needUpdate = true;
            }
            if (needUpdate) {
                NetworkAccessManager::getInstance().cache()->updateMetaData(metaData);
            }
        }
    }
    
    _replyTimer = new QTimer(this);
    connect(_replyTimer, SIGNAL(timeout()), SLOT(handleReplyTimeout()));
    _replyTimer->setSingleShot(true);
    _replyTimer->start(REPLY_TIMEOUT_MS);
    _bytesReceived = _bytesTotal = 0;
}

void Resource::handleReplyError(QNetworkReply::NetworkError error, QDebug debug) {
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
    qCDebug(networking) << "Got finished without download progress/error?" << _url;
    handleDownloadProgress(0, 0);
}

uint qHash(const QPointer<QObject>& value, uint seed) {
    return qHash(value.data(), seed);
}
