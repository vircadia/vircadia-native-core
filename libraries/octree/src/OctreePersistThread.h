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
#include <QtCore/QSharedPointer>
#include <GenericThread.h>
#include "Octree.h"

class OctreePersistThread : public QObject {
    Q_OBJECT
public:
    class BackupRule {
    public:
        QString name;
        int interval;
        QString extensionFormat;
        int maxBackupVersions;
        quint64 lastBackup;
    };

    static const std::chrono::seconds DEFAULT_PERSIST_INTERVAL;

    OctreePersistThread(OctreePointer tree,
                        const QString& filename,
                        std::chrono::milliseconds persistInterval = DEFAULT_PERSIST_INTERVAL,
                        bool debugTimestampNow = false,
                        QString persistAsFileType = "json.gz");

    bool isInitialLoadComplete() const { return _initialLoadComplete; }
    quint64 getLoadElapsedTime() const { return _loadTimeUSecs; }

    QString getPersistFilename() const { return _filename; }
    QString getPersistFileMimeType() const;
    QByteArray getPersistFileContents() const;

    void aboutToFinish(); /// call this to inform the persist thread that the owner is about to finish to support final persist

public slots:
    void start();

signals:
    void loadCompleted();

protected slots:
    void process();
    void handleOctreeDataFileReply(QSharedPointer<ReceivedMessage> message);

protected:
    void persist();
    bool backupCurrentFile();
    void cleanupOldReplacementBackups();

    void replaceData(QByteArray data);
    void sendLatestEntityDataToDS();

private:
    OctreePointer _tree;
    QString _filename;
    std::chrono::milliseconds _persistInterval;
    std::chrono::steady_clock::time_point _lastPersistCheck;
    bool _initialLoadComplete;

    quint64 _loadTimeUSecs;

    bool _debugTimestampNow;
    quint64 _lastTimeDebug;

    QString _persistAsFileType;
    QByteArray _cachedJSONData;
};

#endif // hifi_OctreePersistThread_h
