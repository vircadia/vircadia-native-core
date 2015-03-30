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
#include "Script.h"

class ScriptUser {
public:
    virtual void scriptContentsAvailable(const QUrl& url, const QString& scriptContents) = 0;
    virtual void errorInLoadingScript(const QUrl& url) = 0;
};

/// Interface for loading scripts
class ScriptCache : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    void getScript(const QUrl& url, ScriptUser* scriptUser); // asynchronous
    QString getScript(const QUrl& url); // blocking call
    
private slots:
    void scriptDownloaded();
    void scriptDownloadedSyncronously();

    
private:
    ScriptCache(QObject* parent = NULL);
    
    QHash<QUrl, QString> _scriptCache;
    QMultiMap<QUrl, ScriptUser*> _scriptUsers;
    QSet<QUrl> _blockingScriptsPending;
};

#endif // hifi_ScriptCache_h