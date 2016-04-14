//
//  ResourceCache.h
//  libraries/shared/src
//
//  Created by Andrzej Kapolka on 2/27/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ResourceCache_h
#define hifi_ResourceCache_h

#include <atomic>
#include <mutex>

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QUrl>
#include <QtCore/QWeakPointer>
#include <QtCore/QReadWriteLock>
#include <QtCore/QQueue>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <DependencyManager.h>

#include "ResourceManager.h"

Q_DECLARE_METATYPE(size_t)

class QNetworkReply;
class QTimer;

class Resource;

static const qint64 BYTES_PER_MEGABYTES = 1024 * 1024;
static const qint64 BYTES_PER_GIGABYTES = 1024 * BYTES_PER_MEGABYTES;
static const qint64 MAXIMUM_CACHE_SIZE = 10 * BYTES_PER_GIGABYTES;  // 10GB

// Windows can have troubles allocating that much memory in ram sometimes
// so default cache size at 100 MB on windows (1GB otherwise)
#ifdef Q_OS_WIN32
static const qint64 DEFAULT_UNUSED_MAX_SIZE = 100 * BYTES_PER_MEGABYTES;
#else
static const qint64 DEFAULT_UNUSED_MAX_SIZE = 1024 * BYTES_PER_MEGABYTES;
#endif
static const qint64 MIN_UNUSED_MAX_SIZE = 0;
static const qint64 MAX_UNUSED_MAX_SIZE = 10 * BYTES_PER_GIGABYTES;

// We need to make sure that these items are available for all instances of
// ResourceCache derived classes. Since we can't count on the ordering of
// static members destruction, we need to use this Dependency manager implemented
// object instead
class ResourceCacheSharedItems : public Dependency  {
    SINGLETON_DEPENDENCY

    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;
public:
    void appendPendingRequest(Resource* newRequest);
    void appendActiveRequest(Resource* newRequest);
    void removeRequest(Resource* doneRequest);
    QList<QPointer<Resource>> getPendingRequests() const;
    uint32_t getPendingRequestsCount() const;
    QList<Resource*> getLoadingRequests() const;
    Resource* getHighestPendingRequest();

private:
    ResourceCacheSharedItems() { }
    virtual ~ResourceCacheSharedItems() { }

    mutable Mutex _mutex;
    QList<QPointer<Resource>> _pendingRequests;
    QList<Resource*> _loadingRequests;
};


/// Base class for resource caches.
class ResourceCache : public QObject {
    Q_OBJECT
    Q_PROPERTY(size_t numTotal READ getNumTotalResources NOTIFY dirty)
    Q_PROPERTY(size_t numCached READ getNumCachedResources NOTIFY dirty)
    Q_PROPERTY(size_t sizeTotal READ getSizeTotalResources NOTIFY dirty)
    Q_PROPERTY(size_t sizeCached READ getSizeCachedResources NOTIFY dirty)
    
public:
    size_t getNumTotalResources() const { return _numTotalResources; }
    size_t getSizeTotalResources() const { return _totalResourcesSize; }

    size_t getNumCachedResources() const { return _numUnusedResources; }
    size_t getSizeCachedResources() const { return _unusedResourcesSize; }

    Q_INVOKABLE QVariantList getResourceList();

    static void setRequestLimit(int limit);
    static int getRequestLimit() { return _requestLimit; }

    static int getRequestsActive() { return _requestsActive; }
    
    void setUnusedResourceCacheSize(qint64 unusedResourcesMaxSize);
    qint64 getUnusedResourceCacheSize() const { return _unusedResourcesMaxSize; }

    static const QList<Resource*> getLoadingRequests() 
        { return DependencyManager::get<ResourceCacheSharedItems>()->getLoadingRequests(); }

    static int getPendingRequestCount() 
        { return DependencyManager::get<ResourceCacheSharedItems>()->getPendingRequestsCount(); }

    ResourceCache(QObject* parent = NULL);
    virtual ~ResourceCache();
    
    void refreshAll();
    void refresh(const QUrl& url);

signals:
    void dirty();

public slots:
    void checkAsynchronousGets();

protected slots:
    void updateTotalSize(const qint64& oldSize, const qint64& newSize);

protected:
    /// Loads a resource from the specified URL.
    /// \param fallback a fallback URL to load if the desired one is unavailable
    /// \param delayLoad if true, don't load the resource immediately; wait until load is first requested
    /// \param extra extra data to pass to the creator, if appropriate
    QSharedPointer<Resource> getResource(const QUrl& url, const QUrl& fallback = QUrl(),
                                                     bool delayLoad = false, void* extra = NULL);

    /// Creates a new resource.
    virtual QSharedPointer<Resource> createResource(const QUrl& url,
        const QSharedPointer<Resource>& fallback, bool delayLoad, const void* extra) = 0;
    
    void addUnusedResource(const QSharedPointer<Resource>& resource);
    void removeUnusedResource(const QSharedPointer<Resource>& resource);
    
    /// Attempt to load a resource if requests are below the limit, otherwise queue the resource for loading
    /// \return true if the resource began loading, otherwise false if the resource is in the pending queue
    static bool attemptRequest(Resource* resource);
    static void requestCompleted(Resource* resource);
    static bool attemptHighestPriorityRequest();

private:
    friend class Resource;

    void reserveUnusedResource(qint64 resourceSize);
    void clearUnusedResource();
    void resetResourceCounters();
    void removeResource(const QUrl& url, qint64 size = 0);

    QHash<QUrl, QWeakPointer<Resource>> _resources;
    int _lastLRUKey = 0;
    
    static int _requestLimit;
    static int _requestsActive;

    void getResourceAsynchronously(const QUrl& url);
    QReadWriteLock _resourcesToBeGottenLock;
    QQueue<QUrl> _resourcesToBeGotten;
    
    std::atomic<size_t> _numTotalResources { 0 };
    std::atomic<size_t> _numUnusedResources { 0 };

    std::atomic<qint64> _totalResourcesSize { 0 };
    std::atomic<qint64> _unusedResourcesSize { 0 };

    qint64 _unusedResourcesMaxSize = DEFAULT_UNUSED_MAX_SIZE;
    QMap<int, QSharedPointer<Resource>> _unusedResources;
};

/// Base class for resources.
class Resource : public QObject {
    Q_OBJECT

public:
    
    Resource(const QUrl& url, bool delayLoad = false);
    ~Resource();
    
    /// Returns the key last used to identify this resource in the unused map.
    int getLRUKey() const { return _lruKey; }

    /// Makes sure that the resource has started loading.
    void ensureLoading();

    /// Sets the load priority for one owner.
    virtual void setLoadPriority(const QPointer<QObject>& owner, float priority);
    
    /// Sets a set of priorities at once.
    virtual void setLoadPriorities(const QHash<QPointer<QObject>, float>& priorities);
    
    /// Clears the load priority for one owner.
    virtual void clearLoadPriority(const QPointer<QObject>& owner);
    
    /// Returns the highest load priority across all owners.
    float getLoadPriority();

    /// Checks whether the resource has loaded.
    virtual bool isLoaded() const { return _loaded; }

    /// For loading resources, returns the number of bytes received.
    qint64 getBytesReceived() const { return _bytesReceived; }
    
    /// For loading resources, returns the number of total bytes (<= zero if unknown).
    qint64 getBytesTotal() const { return _bytesTotal; }

    /// For loaded resources, returns the number of actual bytes (defaults to total bytes if not explicitly set).
    qint64 getBytes() const { return _bytes; }

    /// For loading resources, returns the load progress.
    float getProgress() const { return (_bytesTotal <= 0) ? 0.0f : (float)_bytesReceived / _bytesTotal; }
    
    /// Refreshes the resource.
    void refresh();

    void setSelf(const QWeakPointer<Resource>& self) { _self = self; }

    void setCache(ResourceCache* cache) { _cache = cache; }

    virtual void deleter() { allReferencesCleared(); }
    
    const QUrl& getURL() const { return _url; }

signals:
    /// Fired when the resource has been downloaded.
    /// This can be used instead of downloadFinished to access data before it is processed.
    void loaded(const QByteArray request);

    /// Fired when the resource has finished loading.
    void finished(bool success);

    /// Fired when the resource failed to load.
    void failed(QNetworkReply::NetworkError error);

    /// Fired when the resource is refreshed.
    void onRefresh();

protected slots:
    void attemptRequest();

protected:
    virtual void init();

    /// Checks whether the resource is cacheable.
    virtual bool isCacheable() const { return true; }

    /// Called when the download has finished.
    /// This should be overridden by subclasses that need to process the data once it is downloaded.
    virtual void downloadFinished(const QByteArray& data) { finishedLoading(true); }

    /// Called when the download is finished and processed, sets the number of actual bytes.
    void setSize(const qint64& bytes);

    /// Called when the download is finished and processed.
    /// This should be called by subclasses that override downloadFinished to mark the end of processing.
    Q_INVOKABLE void finishedLoading(bool success);

    Q_INVOKABLE void allReferencesCleared();

    QUrl _url;
    QUrl _activeUrl;
    bool _startedLoading = false;
    bool _failedToLoad = false;
    bool _loaded = false;
    QHash<QPointer<QObject>, float> _loadPriorities;
    QWeakPointer<Resource> _self;
    QPointer<ResourceCache> _cache;
    
private slots:
    void handleDownloadProgress(uint64_t bytesReceived, uint64_t bytesTotal);
    void handleReplyFinished();

private:
    void setLRUKey(int lruKey) { _lruKey = lruKey; }
    
    void makeRequest();
    void retry();
    void reinsert();
    
    friend class ResourceCache;
    
    ResourceRequest* _request = nullptr;
    int _lruKey = 0;
    QTimer* _replyTimer = nullptr;
    qint64 _bytesReceived = 0;
    qint64 _bytesTotal = 0;
    qint64 _bytes = 0;
    int _attempts = 0;
};

uint qHash(const QPointer<QObject>& value, uint seed = 0);

#endif // hifi_ResourceCache_h
