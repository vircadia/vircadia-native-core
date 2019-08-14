//
//  DomainContentBackupManager.h
//  libraries/domain-server/src
//
//  Created by Ryan Huffman on 1/01/18.
//  Adapted from OctreePersistThread
//  Copyright 2018 High Fidelity, Inc.
//
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DomainContentBackupManager_h
#define hifi_DomainContentBackupManager_h

#include <RegisteredMetaTypes.h>

#include <QString>
#include <QVector>
#include <QDateTime>
#include <QTimer>

#include <mutex>
#include <unordered_map>

#include <GenericThread.h>

#include "BackupHandler.h"
#include "DomainServerSettingsManager.h"

#include <shared/MiniPromises.h>

#include <PortableHighResolutionClock.h>

const QString DATETIME_FORMAT_RE { "\\d{4}-\\d{2}-\\d{2}_\\d{2}-\\d{2}-\\d{2}" };
const QString AUTOMATIC_BACKUP_PREFIX { "autobackup-" };
const QString MANUAL_BACKUP_PREFIX { "backup-" };
const QString INSTALLED_CONTENT = "installed_content";
const QString INSTALLED_CONTENT_FILENAME = "filename";
const QString INSTALLED_CONTENT_NAME = "name";
const QString INSTALLED_CONTENT_CREATION_TIME = "creation_time";
const QString INSTALLED_CONTENT_INSTALL_TIME = "install_time";
const QString INSTALLED_CONTENT_INSTALLED_BY = "installed_by";

struct BackupItemInfo {
    BackupItemInfo(QString pId, QString pName, QString pAbsolutePath, QDateTime pCreatedAt, bool pIsManualBackup) :
        id(pId), name(pName), absolutePath(pAbsolutePath), createdAt(pCreatedAt), isManualBackup(pIsManualBackup) { };

    QString id;
    QString name;
    QString absolutePath;
    QDateTime createdAt;
    bool isManualBackup;
};

struct ConsolidatedBackupInfo {
    enum State {
        CONSOLIDATING,
        COMPLETE_WITH_ERROR,
        COMPLETE_WITH_SUCCESS
    };
    State state;
    QString error;
    QString absoluteFilePath;
    std::chrono::system_clock::time_point createdAt;
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

    static const std::chrono::seconds DEFAULT_PERSIST_INTERVAL;

    DomainContentBackupManager(const QString& rootBackupDirectory,
                               DomainServerSettingsManager& domainServerSettingsManager,
                               std::chrono::milliseconds persistInterval = DEFAULT_PERSIST_INTERVAL,
                               bool debugTimestampNow = false);

    std::vector<BackupItemInfo> getAllBackups();
    void addBackupHandler(BackupHandlerPointer handler);
    void aboutToFinish();  /// call this to inform the persist thread that the owner is about to finish to support final persist
    void replaceData(QByteArray data);
    ConsolidatedBackupInfo consolidateBackup(QString fileName);

public slots:
    void getAllBackupsAndStatus(MiniPromise::Promise promise);
    void createManualBackup(MiniPromise::Promise promise, const QString& name);
    void recoverFromBackup(MiniPromise::Promise promise, const QString& backupName, const QString& username);
    void recoverFromUploadedBackup(MiniPromise::Promise promise, QByteArray uploadedBackup, QString username);
    void recoverFromUploadedFile(MiniPromise::Promise promise, QString uploadedFilename, QString username, QString sourceFilename);
    void deleteBackup(MiniPromise::Promise promise, const QString& backupName);

signals:
    void loadCompleted();
    void recoveryCompleted();

protected:
    /// Implements generic processing behavior for this thread.
    virtual void setup() override;
    virtual bool process() override;
    virtual void shutdown() override;

    void backup();
    void removeOldBackupVersions(const BackupRule& rule);
    void refreshBackupRules();
    bool getMostRecentBackup(const QString& format, QString& mostRecentBackupFileName, QDateTime& mostRecentBackupTime);
    int64_t getMostRecentBackupTimeInSecs(const QString& format);
    void parseBackupRules(const QVariantList& backupRules);

    std::pair<bool, QString> createBackup(const QString& prefix, const QString& name);

    bool recoverFromBackupZip(const QString& backupName, QuaZip& backupZip, const QString& username, const QString& sourceFilename, bool rollingBack = false);

private slots:
    void removeOldConsolidatedBackups();
    void consolidateBackupInternal(QString fileName);

private:
    DomainServerSettingsManager& _settingsManager;

    QTimer _consolidatedBackupCleanupTimer;

    const QString _consolidatedBackupDirectory;
    const QString _backupDirectory;
    std::vector<BackupHandlerPointer> _backupHandlers;
    std::chrono::milliseconds _persistInterval { 0 };

    std::mutex _consolidatedBackupsMutex;
    std::unordered_map<QString, ConsolidatedBackupInfo> _consolidatedBackups;

    std::atomic<bool> _isRecovering { false };
    QString _recoveryError;
    QString _recoveryFilename { };

    p_high_resolution_clock::time_point _lastCheck;
    std::vector<BackupRule> _backupRules;
};

#endif  // hifi_DomainContentBackupManager_h
