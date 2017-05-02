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
    class BackupRule {
    public:
        QString name;
        int interval;
        QString extensionFormat;
        int maxBackupVersions;
        quint64 lastBackup;
    };

    static const int DEFAULT_PERSIST_INTERVAL;
    static const QString REPLACEMENT_FILE_EXTENSION;

    OctreePersistThread(OctreePointer tree, const QString& filename, const QString& backupDirectory,
                        int persistInterval = DEFAULT_PERSIST_INTERVAL, bool wantBackup = false,
                        const QJsonObject& settings = QJsonObject(), bool debugTimestampNow = false, QString persistAsFileType="json.gz");

    bool isInitialLoadComplete() const { return _initialLoadComplete; }
    quint64 getLoadElapsedTime() const { return _loadTimeUSecs; }

    void aboutToFinish(); /// call this to inform the persist thread that the owner is about to finish to support final persist

    QString getPersistFilename() const { return _filename; }
    QString getPersistFileMimeType() const;
    QByteArray getPersistFileContents() const;

signals:
    void loadCompleted();

protected:
    /// Implements generic processing behavior for this thread.
    virtual bool process() override;

    void persist();
    void backup();
    void rollOldBackupVersions(const BackupRule& rule);
    void restoreFromMostRecentBackup();
    bool getMostRecentBackup(const QString& format, QString& mostRecentBackupFileName, QDateTime& mostRecentBackupTime);
    quint64 getMostRecentBackupTimeInUsecs(const QString& format);
    void parseSettings(const QJsonObject& settings);
    void possiblyReplaceContent();

private:
    OctreePointer _tree;
    QString _filename;
    QString _backupDirectory;
    int _persistInterval;
    bool _initialLoadComplete;

    quint64 _loadTimeUSecs;

    time_t _lastPersistTime;
    quint64 _lastCheck;
    bool _wantBackup;
    QVector<BackupRule> _backupRules;

    bool _debugTimestampNow;
    quint64 _lastTimeDebug;

    QString _persistAsFileType;
};

#endif // hifi_OctreePersistThread_h
