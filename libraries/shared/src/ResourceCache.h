//
//  ResourceCache.h
//  shared
//
//  Created by Andrzej Kapolka on 2/27/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __shared__ResourceCache__
#define __shared__ResourceCache__

#include <QHash>
#include <QList>
#include <QNetworkRequest>
#include <QObject>
#include <QPointer>
#include <QSharedPointer>
#include <QUrl>
#include <QWeakPointer>

class QNetworkAccessManager;
class QNetworkReply;

class Resource;

/// Base class for resource caches.
class ResourceCache : public QObject {
    Q_OBJECT
    
public:

    static void setNetworkAccessManager(QNetworkAccessManager* manager) { _networkAccessManager = manager; }
    static QNetworkAccessManager* getNetworkAccessManager() { return _networkAccessManager; }

    static void setRequestLimit(int limit) { _requestLimit = limit; }
    static int getRequestLimit() { return _requestLimit; }

    ResourceCache(QObject* parent = NULL);

protected:

    /// Loads a resource from the specified URL.
    /// \param fallback a fallback URL to load if the desired one is unavailable
    /// \param delayLoad if true, don't load the resource immediately; wait until load is first requested
    /// \param extra extra data to pass to the creator, if appropriate
    QSharedPointer<Resource> getResource(const QUrl& url, const QUrl& fallback = QUrl(),
        bool delayLoad = false, void* extra = NULL);

    /// Creates a new resource.
    virtual QSharedPointer<Resource> createResource(const QUrl& url,
        const QSharedPointer<Resource>& fallback, bool delayLoad, void* extra) = 0;

    static void attemptRequest(Resource* resource);
    static void requestCompleted();

private:
    
    friend class Resource;
    
    QHash<QUrl, QWeakPointer<Resource> > _resources;
    
    static QNetworkAccessManager* _networkAccessManager;
    static int _requestLimit;
    static QList<QPointer<Resource> > _pendingRequests;
};

/// Base class for resources.
class Resource : public QObject {
    Q_OBJECT

public:
    
    Resource(const QUrl& url, bool delayLoad = false);
    ~Resource();
    
    /// Makes sure that the resource has started loading.
    void ensureLoading();

    /// Sets the load priority for one owner.
    void setLoadPriority(const QPointer<QObject>& owner, float priority) { _loadPriorities.insert(owner, priority); }
    
    /// Clears the load priority for one owner.
    void clearLoadPriority(const QPointer<QObject>& owner) { _loadPriorities.remove(owner); }
    
    /// Returns the highest load priority across all owners.
    float getLoadPriority();

protected slots:

    void attemptRequest();

protected:

    virtual void downloadFinished(QNetworkReply* reply) = 0;

    QNetworkRequest _request;
    bool _startedLoading;
    bool _failedToLoad;
    
private slots:
    
    void handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleReplyError();

private:
    
    void makeRequest();
    
    friend class ResourceCache;
    
    QNetworkReply* _reply;
    int _attempts;
    
    QHash<QPointer<QObject>, float> _loadPriorities;
};

uint qHash(const QPointer<QObject>& value, uint seed = 0);

#endif /* defined(__shared__ResourceCache__) */
