//
//  OctreePersistThread.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 8/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Threaded or non-threaded Octree persistence
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreePersistThread_h
#define hifi_OctreePersistThread_h

#include <QString>
#include <GenericThread.h>
#include "Octree.h"

/// Generalized threaded processor for handling received inbound packets.
class OctreePersistThread : public GenericThread {
    Q_OBJECT
public:
    static const int DEFAULT_PERSIST_INTERVAL;
    static const int DEFAULT_BACKUP_INTERVAL;
    static const QString DEFAULT_BACKUP_EXTENSION_FORMAT;
    static const int DEFAULT_MAX_BACKUP_VERSIONS;

    OctreePersistThread(Octree* tree, const QString& filename, int persistInterval = DEFAULT_PERSIST_INTERVAL, 
                                bool wantBackup = false, int backupInterval = DEFAULT_BACKUP_INTERVAL,
                                const QString& backupExtensionFormat = DEFAULT_BACKUP_EXTENSION_FORMAT,
                                int maxBackupVersions = DEFAULT_MAX_BACKUP_VERSIONS);

    bool isInitialLoadComplete() const { return _initialLoadComplete; }
    quint64 getLoadElapsedTime() const { return _loadTimeUSecs; }

    void aboutToFinish(); /// call this to inform the persist thread that the owner is about to finish to support final persist

signals:
    void loadCompleted();

protected:
    /// Implements generic processing behavior for this thread.
    virtual bool process();
    
    void persist();
    void backup();
    void rollOldBackupVersions();
private:
    Octree* _tree;
    QString _filename;
    QString _backupExtensionFormat;
    int _maxBackupVersions;
    int _persistInterval;
    int _backupInterval;
    bool _initialLoadComplete;

    quint64 _loadTimeUSecs;
    quint64 _lastCheck;
    quint64 _lastBackup;
    bool _wantBackup;
    time_t _lastPersistTime;
};

#endif // hifi_OctreePersistThread_h
