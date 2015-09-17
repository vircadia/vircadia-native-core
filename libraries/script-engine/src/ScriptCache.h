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

#include <ResourceCache.h>

class ScriptUser {
public:
    virtual void scriptContentsAvailable(const QUrl& url, const QString& scriptContents) = 0;
    virtual void errorInLoadingScript(const QUrl& url) = 0;
};

using contentAvailableCallback = std::function<void(const QString& scriptOrURL, const QString& contents, bool isURL, bool contentAvailable)>;

/// Interface for loading scripts
class ScriptCache : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

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
    
    QMultiMap<QUrl, contentAvailableCallback> _contentCallbacks;
    
    QHash<QUrl, QString> _scriptCache;
    QMultiMap<QUrl, ScriptUser*> _scriptUsers;
    QSet<QUrl> _badScripts;
};

#endif // hifi_ScriptCache_h