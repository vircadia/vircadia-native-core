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
#include <QVector>
#include <QDateTime>

#include <GenericThread.h>

#include "BackupHandler.h"

#include <shared/MiniPromises.h>

struct BackupItemInfo {
    BackupItemInfo(QString pId, QString pName, QString pAbsolutePath, QDateTime pCreatedAt, bool pIsManualBackup) :
        id(pId), name(pName), absolutePath(pAbsolutePath), createdAt(pCreatedAt), isManualBackup(pIsManualBackup) { };

    QString id;
    QString name;
    QString absolutePath;
    QDateTime createdAt;
    bool isManualBackup;
};

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

    std::vector<BackupItemInfo> getAllBackups();
    void addBackupHandler(BackupHandlerPointer handler);
    void aboutToFinish();  /// call this to inform the persist thread that the owner is about to finish to support final persist
    void replaceData(QByteArray data);

public slots:
    void getAllBackupsAndStatus(MiniPromise::Promise promise);
    void createManualBackup(MiniPromise::Promise promise, const QString& name);
    void recoverFromBackup(MiniPromise::Promise promise, const QString& backupName);
    void recoverFromUploadedBackup(MiniPromise::Promise promise, QByteArray uploadedBackup);
    void deleteBackup(MiniPromise::Promise promise, const QString& backupName);
    void consolidateBackup(MiniPromise::Promise promise, QString fileName);

signals:
    void loadCompleted();
    void recoveryCompleted();

protected:
    /// Implements generic processing behavior for this thread.
    virtual void setup() override;
    virtual bool process() override;

    void load();
    void backup();
    void removeOldBackupVersions(const BackupRule& rule);
    void refreshBackupRules();
    bool getMostRecentBackup(const QString& format, QString& mostRecentBackupFileName, QDateTime& mostRecentBackupTime);
    int64_t getMostRecentBackupTimeInSecs(const QString& format);
    void parseSettings(const QJsonObject& settings);

    std::pair<bool, QString> createBackup(const QString& prefix, const QString& name);

    bool recoverFromBackupZip(QuaZip& backupZip, const QString& backupName);

private:
    const QString _backupDirectory;
    std::vector<BackupHandlerPointer> _backupHandlers;
    int _persistInterval { 0 };

    std::atomic<bool> _isRecovering { false };
    QString _recoveryFilename { };

    int64_t _lastCheck { 0 };
    std::vector<BackupRule> _backupRules;
};

#endif  // hifi_DomainContentBackupManager_h
