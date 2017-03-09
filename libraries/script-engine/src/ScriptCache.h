//
//  ScriptCache.h
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 2015-03-30
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptCache_h
#define hifi_ScriptCache_h

#include <mutex>
#include <ResourceCache.h>

using contentAvailableCallback = std::function<void(const QString& scriptOrURL, const QString& contents, bool isURL, bool contentAvailable, const QString& status)>;

class ScriptUser {
public:
    virtual void scriptContentsAvailable(const QUrl& url, const QString& scriptContents) = 0;
    virtual void errorInLoadingScript(const QUrl& url) = 0;
};

class ScriptRequest {
public:
    static const int MAX_RETRIES { 5 };
    static const int START_DELAY_BETWEEN_RETRIES { 200 };
    std::vector<contentAvailableCallback> scriptUsers { };
    int numRetries { 0 };
    int maxRetries { MAX_RETRIES };
};

/// Interface for loading scripts
class ScriptCache : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;

public:
    static const QString STATUS_INLINE;
    static const QString STATUS_CACHED;
    static bool isSuccessStatus(const QString& status) {
        return status == "Success" || status == STATUS_INLINE || status == STATUS_CACHED;
    }

    void clearCache();
    Q_INVOKABLE void clearATPScriptsFromCache();
    void getScriptContents(const QString& scriptOrURL, contentAvailableCallback contentAvailable, bool forceDownload = false, int maxRetries = ScriptRequest::MAX_RETRIES);

    void deleteScript(const QUrl& unnormalizedURL);

private:
    void scriptContentAvailable(int maxRetries); // new version
    ScriptCache(QObject* parent = NULL);
    
    Mutex _containerLock;
    QMap<QUrl, ScriptRequest> _activeScriptRequests;
    
    QHash<QUrl, QString> _scriptCache;
    QMultiMap<QUrl, ScriptUser*> _scriptUsers;
};

#endif // hifi_ScriptCache_h
