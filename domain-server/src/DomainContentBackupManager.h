//
//  DomainContentBackupManager.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 8/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DomainContentBackupManager_h
#define hifi_DomainContentBackupManager_h

#include <QString>
#include <GenericThread.h>
#include <QVector>

#include <quazip5/quazip.h>
#include <quazip5/quazipfile.h>

#include <functional>

using BackupResult = std::vector<QString>;
using CreateBackupHandler = std::function<void(QuaZip* quazip)>;
using RecoverBackupHandler = std::function<void()>;

class DomainContentBackupManager : public GenericThread {
    Q_OBJECT
public:
    class BackupRule {
    public:
        QString name;
        int intervalSeconds;
        QString extensionFormat;
        int maxBackupVersions;
        qint64 lastBackupSeconds;
    };

    static const int DEFAULT_PERSIST_INTERVAL;

    DomainContentBackupManager(const QString& rootBackupDirectory,
                               const QJsonObject& settings,
                               int persistInterval = DEFAULT_PERSIST_INTERVAL,
                               bool debugTimestampNow = false);

    void addCreateBackupHandler(CreateBackupHandler handler);
    bool isInitialLoadComplete() const { return _initialLoadComplete; }
    int64_t getLoadElapsedTime() const { return _loadTimeUSecs; }

    void aboutToFinish();  /// call this to inform the persist thread that the owner is about to finish to support final persist

    void replaceData(QByteArray data);

signals:
    void loadCompleted();

protected:
    /// Implements generic processing behavior for this thread.
    bool process() override;

    void persist();
    void backup();
    void removeOldBackupVersions(const BackupRule& rule);
    bool getMostRecentBackup(const QString& format, QString& mostRecentBackupFileName, QDateTime& mostRecentBackupTime);
    int64_t getMostRecentBackupTimeInSecs(const QString& format);
    void parseSettings(const QJsonObject& settings);

private:
    QString _backupDirectory;
    std::vector<CreateBackupHandler> _backupHandlers;
    int _persistInterval;
    bool _initialLoadComplete;

    int64_t _loadTimeUSecs;

    time_t _lastPersistTime;
    int64_t _lastCheck;
    bool _wantBackup{ true };
    QVector<BackupRule> _backupRules;

    bool _debugTimestampNow;
    int64_t _lastTimeDebug;
};

#endif  // hifi_DomainContentBackupManager_h
