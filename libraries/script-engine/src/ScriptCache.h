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

using contentAvailableCallback = std::function<void(const QString& scriptOrURL, const QString& contents, bool isURL, bool contentAvailable)>;

class ScriptUser {
public:
    virtual void scriptContentsAvailable(const QUrl& url, const QString& scriptContents) = 0;
    virtual void errorInLoadingScript(const QUrl& url) = 0;
};

class ScriptRequest {
public:
    std::vector<contentAvailableCallback> scriptUsers { };
    int numRetries { 0 };
};

/// Interface for loading scripts
class ScriptCache : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;

public:
    void clearCache();
    void getScriptContents(const QString& scriptOrURL, contentAvailableCallback contentAvailable, bool forceDownload = false);


    QString getScript(const QUrl& unnormalizedURL, ScriptUser* scriptUser, bool& isPending, bool redownload = false);
    void deleteScript(const QUrl& unnormalizedURL);

    // FIXME - how do we remove a script from the bad script list in the case of a redownload?
    void addScriptToBadScriptList(const QUrl& url) { _badScripts.insert(url); }
    bool isInBadScriptList(const QUrl& url) { return _badScripts.contains(url); }
    
private slots:
    void scriptDownloaded(); // old version
    void scriptContentAvailable(); // new version

private:
    ScriptCache(QObject* parent = NULL);
    
    Mutex _containerLock;
    QMap<QUrl, ScriptRequest> _activeScriptRequests;
    //QMultiMap<QUrl, contentAvailableCallback> _contentCallbacks;
    
    QHash<QUrl, QString> _scriptCache;
    QMultiMap<QUrl, ScriptUser*> _scriptUsers;
    QSet<QUrl> _badScripts;
};

#endif // hifi_ScriptCache_h