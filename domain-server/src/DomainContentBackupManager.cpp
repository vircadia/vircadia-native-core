//
//  DomainContentBackupManager.cpp
//  libraries/domain-server/src
//
//  Created by Ryan Huffman on 1/01/18.
//  Adapted from OctreePersistThread
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DomainContentBackupManager.h"

#include <chrono>
#include <thread>

#include <cstdio>
#include <fstream>
#include <time.h>

#include <QBuffer>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include <quazip5/quazip.h>

#include <NumericalConstants.h>
#include <PerfStat.h>
#include <PathUtils.h>
#include <shared/QtHelpers.h>

#include "DomainServer.h"

const std::chrono::seconds DomainContentBackupManager::DEFAULT_PERSIST_INTERVAL { 30 };

// Backup format looks like: daily_backup-TIMESTAMP.zip
static const QString DATETIME_FORMAT { "yyyy-MM-dd_HH-mm-ss" };
static const QString PRE_UPLOAD_SUFFIX{ "pre_upload" };
static const QString MANUAL_BACKUP_NAME_RE { "[a-zA-Z0-9\\-_ ]+" };

void DomainContentBackupManager::addBackupHandler(BackupHandlerPointer handler) {
    _backupHandlers.push_back(std::move(handler));
}

DomainContentBackupManager::DomainContentBackupManager(const QString& backupDirectory,
                                                       DomainServerSettingsManager& domainServerSettingsManager,
                                                       std::chrono::milliseconds persistInterval,
                                                       bool debugTimestampNow) :
    _settingsManager(domainServerSettingsManager),
    _consolidatedBackupDirectory(PathUtils::generateTemporaryDir()),
    _backupDirectory(backupDirectory), _persistInterval(persistInterval), _lastCheck(p_high_resolution_clock::now())
{
    setObjectName("DomainContentBackupManager");

    // Make sure the backup directory exists.
    QDir(_backupDirectory).mkpath(".");

    static const QString BACKUP_RULES_KEYPATH = AUTOMATIC_CONTENT_ARCHIVES_GROUP + ".backup_rules";
    parseBackupRules(_settingsManager.valueOrDefaultValueForKeyPath(BACKUP_RULES_KEYPATH).toList());

    constexpr int CONSOLIDATED_BACKUP_CLEANER_INTERVAL_MSECS = 30 * 1000;
    _consolidatedBackupCleanupTimer.setInterval(CONSOLIDATED_BACKUP_CLEANER_INTERVAL_MSECS);
    connect(&_consolidatedBackupCleanupTimer, &QTimer::timeout, this, &DomainContentBackupManager::removeOldConsolidatedBackups);
    _consolidatedBackupCleanupTimer.start();
}

void DomainContentBackupManager::parseBackupRules(const QVariantList& backupRules) {
    qCDebug(domain_server) << "BACKUP RULES:";

    for (const QVariant& value : backupRules) {
        QVariantMap map = value.toMap();

        int interval = map["backupInterval"].toInt();
        int count = map["maxBackupVersions"].toInt();
        auto name = map["Name"].toString();
        auto format = name.toLower();
        QRegExp matchDisallowedCharacters { "[^a-zA-Z0-9\\-_]+" };
        format.replace(matchDisallowedCharacters, "_");

        qCDebug(domain_server) << "    Name:" << name;
        qCDebug(domain_server) << "        format:" << format;
        qCDebug(domain_server) << "        interval:" << interval;
        qCDebug(domain_server) << "        count:" << count;

        BackupRule newRule = { name, interval, format, count, 0 };

        newRule.lastBackupSeconds = getMostRecentBackupTimeInSecs(format);

        if (newRule.lastBackupSeconds > 0) {
            auto now = QDateTime::currentSecsSinceEpoch();
            auto sinceLastBackup = now - newRule.lastBackupSeconds;
            qCDebug(domain_server).noquote() << "        lastBackup:" <<  formatSecTime(sinceLastBackup) << "ago";
        } else {
            qCDebug(domain_server) << "        lastBackup: NEVER";
        }

        _backupRules.push_back(newRule);
    }
}

void DomainContentBackupManager::refreshBackupRules() {
    for (auto& backup : _backupRules) {
        backup.lastBackupSeconds = getMostRecentBackupTimeInSecs(backup.extensionFormat);
    }
}

int64_t DomainContentBackupManager::getMostRecentBackupTimeInSecs(const QString& format) {
    int64_t mostRecentBackupInSecs = 0;

    QString mostRecentBackupFileName;
    QDateTime mostRecentBackupTime;

    bool recentBackup = getMostRecentBackup(format, mostRecentBackupFileName, mostRecentBackupTime);

    if (recentBackup) {
        mostRecentBackupInSecs = mostRecentBackupTime.toSecsSinceEpoch();
    }

    return mostRecentBackupInSecs;
}

void DomainContentBackupManager::setup() {
    for (auto& rule : _backupRules) {
        removeOldBackupVersions(rule);
    }

    auto backups = getAllBackups();
    for (auto& backup : backups) {
        QFile backupFile { backup.absolutePath };
        if (!backupFile.open(QIODevice::ReadOnly)) {
            qCritical() << "Could not open file:" << backup.absolutePath;
            qCritical() << "    ERROR:" << backupFile.errorString();
            continue;
        }

        QuaZip zip { &backupFile };
        if (!zip.open(QuaZip::mdUnzip)) {
            qCritical() << "Could not open backup archive:" << backup.absolutePath;
            qCritical() << "    ERROR:" << zip.getZipError();
            continue;
        }

        for (auto& handler : _backupHandlers) {
            handler->loadBackup(backup.id, zip);
        }

        zip.close();
    }

    for (auto& handler : _backupHandlers) {
        handler->loadingComplete();
    }
}

bool DomainContentBackupManager::process() {
    if (isStillRunning()) {
        constexpr int64_t MSECS_TO_USECS = 1000;
        constexpr int64_t USECS_TO_SLEEP = 10 * MSECS_TO_USECS;  // every 10ms
        std::this_thread::sleep_for(std::chrono::microseconds(USECS_TO_SLEEP));

        if (_isRecovering) {
            bool isStillRecovering = any_of(begin(_backupHandlers), end(_backupHandlers), [](const BackupHandlerPointer& handler) {
                return handler->getRecoveryStatus().first;
            });

            // if an error occurred, don't restart the server so that the user
            // can be notified of the error and take action.
            if (!isStillRecovering && _recoveryError.isEmpty()) {
                _isRecovering = false;
                _recoveryFilename = "";
                emit recoveryCompleted();
            }
        }

        auto now = p_high_resolution_clock::now();
        auto sinceLastSave = now - _lastCheck;
        if (sinceLastSave > _persistInterval) {
            _lastCheck = now;

            if (!_isRecovering) {
                backup();
            }
        }
    }

    return isStillRunning();
}

void DomainContentBackupManager::shutdown() {
    // Destroy handlers on the correct thread so that they can cleanup timers
    _backupHandlers.clear();
}

void DomainContentBackupManager::aboutToFinish() {
    _stopThread = true;
}

bool DomainContentBackupManager::getMostRecentBackup(const QString& format,
                                                     QString& mostRecentBackupFileName,
                                                     QDateTime& mostRecentBackupTime) {
    QRegExp formatRE { AUTOMATIC_BACKUP_PREFIX + QRegExp::escape(format) + "\\-(" + DATETIME_FORMAT_RE + ")" + "\\.zip" };

    QStringList filters;
    filters << AUTOMATIC_BACKUP_PREFIX + format + "*.zip";

    bool bestBackupFound = false;
    QString bestBackupFile;
    QDateTime bestBackupFileTime;

    // Iterate over all of the backup files in the persist location
    QDirIterator dirIterator(_backupDirectory, filters, QDir::Files | QDir::NoSymLinks, QDirIterator::NoIteratorFlags);
    while (dirIterator.hasNext()) {
        dirIterator.next();
        auto fileName = dirIterator.fileInfo().fileName();

        if (formatRE.exactMatch(fileName)) {
            auto datetime = formatRE.cap(1);
            auto createdAt = QDateTime::fromString(datetime, DATETIME_FORMAT);

            if (!createdAt.isValid()) {
                qDebug() << "Skipping backup with invalid timestamp: " << datetime;
                continue;
            }

            qDebug() << "Checking " << dirIterator.fileInfo().filePath();

            // Based on last modified date, track the most recently modified file as the best backup
            if (createdAt > bestBackupFileTime) {
                bestBackupFound = true;
                bestBackupFile = dirIterator.filePath();
                bestBackupFileTime = createdAt;
            }
        } else {
            qDebug() << "NO match: " << fileName << formatRE;
        }
    }

    // If we found a backup then return the results
    if (bestBackupFound) {
        mostRecentBackupFileName = bestBackupFile;
        mostRecentBackupTime = bestBackupFileTime;
    }
    return bestBackupFound;
}

void DomainContentBackupManager::deleteBackup(MiniPromise::Promise promise, const QString& backupName) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "deleteBackup", Q_ARG(MiniPromise::Promise, promise),
                                  Q_ARG(const QString&, backupName));
        return;
    }

    if (_isRecovering && backupName == _recoveryFilename) {
        promise->resolve({
            { "success", false }
        });
        return;
    }

    QDir backupDir { _backupDirectory };
    QFile backupFile { backupDir.filePath(backupName) };
    auto success = backupFile.remove();

    refreshBackupRules();

    for (auto& handler : _backupHandlers) {
        handler->deleteBackup(backupName);
    }

    promise->resolve({
        { "success", success }
    });
}

bool DomainContentBackupManager::recoverFromBackupZip(const QString& backupName, QuaZip& zip, const QString& username, const QString& sourceFilename, bool rollingBack) {
    if (!zip.open(QuaZip::Mode::mdUnzip)) {
        qWarning() << "Failed to unzip file: " << backupName;
        return false;
    } else {
        _isRecovering = true;
        _recoveryFilename = backupName;

        for (auto& handler : _backupHandlers) {
            bool success;
            QString errorStr;
            std::tie(success, errorStr) = handler->recoverBackup(backupName, zip, username, sourceFilename);
            if (!success) {
                if (!rollingBack) {
                    _recoveryError = errorStr;
                }
                return false;
            }
        }

        qDebug() << "Successfully started recovering from " << backupName;
        return true;
    }
}

void DomainContentBackupManager::recoverFromBackup(MiniPromise::Promise promise, const QString& backupName, const QString& username) {
    if (_isRecovering) {
        promise->resolve({
            { "success", false }
        });
        return;
    };

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "recoverFromBackup", Q_ARG(MiniPromise::Promise, promise),
                                  Q_ARG(const QString&, backupName), Q_ARG(const QString&, username));
        return;
    }

    qDebug() << "Recovering from" << backupName;
    _recoveryError = "";
    bool success { false };
    QDir backupDir { _backupDirectory };
    auto backupFilePath { backupDir.filePath(backupName) };
    QFile backupFile { backupFilePath };
    if (backupFile.open(QIODevice::ReadOnly)) {
        QuaZip zip { &backupFile };

        success = recoverFromBackupZip(backupName, zip, username, backupName);

        backupFile.close();
    } else {
        success = false;
        qWarning() << "Failed to open backup file for reading: " << backupFilePath;
    }

    promise->resolve({
        { "success", success }
    });
}

void DomainContentBackupManager::recoverFromUploadedBackup(MiniPromise::Promise promise, QByteArray uploadedBackup, QString username) {

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "recoverFromUploadedBackup", Q_ARG(MiniPromise::Promise, promise),
                                  Q_ARG(QByteArray, uploadedBackup), Q_ARG(QString, username));
        return;
    }

    qDebug() << "Recovering from uploaded content archive";

    // create a buffer and then a QuaZip from that buffer
    QBuffer uploadedBackupBuffer { &uploadedBackup };
    QuaZip uploadedZip { &uploadedBackupBuffer };

    QString backupName = MANUAL_BACKUP_PREFIX + "uploaded.zip";
    bool success = recoverFromBackupZip(backupName, uploadedZip, username, QString());

    promise->resolve({
        { "success", success }
    });
}

void DomainContentBackupManager::recoverFromUploadedFile(MiniPromise::Promise promise, QString uploadedFilename, const QString username, QString sourceFilename) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "recoverFromUploadedFile", Q_ARG(MiniPromise::Promise, promise),
            Q_ARG(QString, uploadedFilename), Q_ARG(QString, username), Q_ARG(QString, sourceFilename));
        return;
    }

    qDebug() << "Recovering from uploaded file -" << uploadedFilename << "source" << sourceFilename;
    bool success;
    QString path;
    std::tie(success, path) = createBackup(AUTOMATIC_BACKUP_PREFIX, PRE_UPLOAD_SUFFIX);
    if(!success) {
        _recoveryError = "Failed to create backup for " + PRE_UPLOAD_SUFFIX + " at " + path;
        qCWarning(domain_server) << _recoveryError;
    } else {
        QFile uploadedFile(uploadedFilename);
        QuaZip uploadedZip { &uploadedFile };

        QString backupName = MANUAL_BACKUP_PREFIX + "uploaded.zip";

        bool success = recoverFromBackupZip(backupName, uploadedZip, username, sourceFilename);

        if (!success) {

            // attempt to rollback to
            QString filename;
            QDateTime filetime;
            if (getMostRecentBackup(PRE_UPLOAD_SUFFIX, filename, filetime)) {
                QFile uploadedFile(uploadedFilename);
                QuaZip uploadedZip { &uploadedFile };

                QString backupName = MANUAL_BACKUP_PREFIX + "uploaded.zip";
                recoverFromBackupZip(backupName, uploadedZip, username, sourceFilename,  true);

            }
        }

    }
    promise->resolve({
        { "success", success }
        });
}

std::vector<BackupItemInfo> DomainContentBackupManager::getAllBackups() {

    QDir backupDir { _backupDirectory };
    auto matchingFiles =
            backupDir.entryInfoList({ AUTOMATIC_BACKUP_PREFIX + "*.zip", MANUAL_BACKUP_PREFIX + "*.zip" },
                                    QDir::Files | QDir::NoSymLinks, QDir::Name);
    QString prefixFormat = "(" + QRegExp::escape(AUTOMATIC_BACKUP_PREFIX) + "|" + QRegExp::escape(MANUAL_BACKUP_PREFIX) + ")";
    QString nameFormat = "(.+)";
    QString dateTimeFormat = "(" + DATETIME_FORMAT_RE + ")";
    QRegExp backupNameFormat { prefixFormat + nameFormat + "-" + dateTimeFormat + "\\.zip" };

    std::vector<BackupItemInfo> backups;

    for (const auto& fileInfo : matchingFiles) {
        auto fileName = fileInfo.fileName();
        if (backupNameFormat.exactMatch(fileName)) {
            auto type = backupNameFormat.cap(1);
            auto name = backupNameFormat.cap(2);
            auto dateTime = backupNameFormat.cap(3);
            auto createdAt = QDateTime::fromString(dateTime, DATETIME_FORMAT);
            if (!createdAt.isValid()) {
                qDebug().nospace() << "Skipping backup (" << fileName << ") with invalid timestamp: " << dateTime;
                continue;
            }

            backups.emplace_back(fileInfo.fileName(), name, fileInfo.absoluteFilePath(), createdAt,
                                 type == MANUAL_BACKUP_PREFIX);
        }
    }

    return backups;
}

void DomainContentBackupManager::getAllBackupsAndStatus(MiniPromise::Promise promise) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "getAllBackupsAndStatus", Q_ARG(MiniPromise::Promise, promise));
        return;
    }

    auto backups = getAllBackups();

    QVariantList variantBackups;

    for (auto& backup : backups) {
        bool isAvailable { true };
        bool isCorrupted { false };
        float availabilityProgress { 0.0f };
        for (auto& handler : _backupHandlers) {
            bool handlerIsAvailable { true };
            float progress { 0.0f };
            std::tie(handlerIsAvailable, progress) = handler->isAvailable(backup.id);
            isAvailable &= handlerIsAvailable;
            availabilityProgress += progress / _backupHandlers.size();

            isCorrupted = isCorrupted || handler->isCorruptedBackup(backup.id);
        }
        variantBackups.push_back(QVariantMap({
            { "id", backup.id },
            { "name", backup.name },
            { "createdAtMillis", backup.createdAt.toMSecsSinceEpoch() },
            { "isAvailable", isAvailable },
            { "availabilityProgress", availabilityProgress },
            { "isManualBackup", backup.isManualBackup },
            { "isCorrupted", isCorrupted }
        }));
    }

    float recoveryProgress = 0.0f;
    bool isRecovering = _isRecovering.load();
    if (_isRecovering) {
        for (auto& handler : _backupHandlers) {
            float progress = handler->getRecoveryStatus().second;
            recoveryProgress += progress / _backupHandlers.size();
        }
    }

    QVariantMap status { 
        { "isRecovering", isRecovering },
        { "recoveringBackupId", _recoveryFilename },
        { "recoveryProgress", recoveryProgress }
    };

    if(!_recoveryError.isEmpty()) {
        status["recoveryError"] = _recoveryError;
    }


    QString filename = _settingsManager.valueForKeyPath(CONTENT_SETTINGS_INSTALLED_CONTENT_FILENAME).toString();
    QString name = _settingsManager.valueForKeyPath(CONTENT_SETTINGS_INSTALLED_CONTENT_NAME).toString();
    auto creationTime = _settingsManager.valueForKeyPath(CONTENT_SETTINGS_INSTALLED_CONTENT_CREATION_TIME).toULongLong();

    if (name.isEmpty() || creationTime == 0) {
        QString prefixFormat = "(" + QRegExp::escape(AUTOMATIC_BACKUP_PREFIX) + "|" + QRegExp::escape(MANUAL_BACKUP_PREFIX) + ")";
        QString nameFormat = "(.+)";
        QString dateTimeFormat = "(" + DATETIME_FORMAT_RE + ")";
        QRegExp backupNameFormat { prefixFormat + nameFormat + "-" + dateTimeFormat + "\\.zip" };


        if (backupNameFormat.exactMatch(filename)) {
            if (name.isEmpty()) {
                name = backupNameFormat.cap(2);
            }
            if (creationTime == 0) {
                auto dateTime = backupNameFormat.cap(3);
                creationTime = QDateTime::fromString(dateTime, DATETIME_FORMAT).toMSecsSinceEpoch();
            }
        }
    }

    QVariantMap currentArchive;
    currentArchive["filename"] = filename;
    currentArchive["name"] = name;
    currentArchive["creation_time"] = creationTime;
    currentArchive["install_time"] = _settingsManager.valueForKeyPath(CONTENT_SETTINGS_INSTALLED_CONTENT_INSTALL_TIME).toULongLong();
    currentArchive["installed_by"] = _settingsManager.valueForKeyPath(CONTENT_SETTINGS_INSTALLED_CONTENT_INSTALLED_BY).toString();

    QVariantMap info {
        { "backups", variantBackups },
        { "status", status },
        { "installed_content", currentArchive }
    };

    promise->resolve(info);
}

void DomainContentBackupManager::removeOldBackupVersions(const BackupRule& rule) {
    QDir backupDir { _backupDirectory };
    if (backupDir.exists() && rule.maxBackupVersions > 0) {

        auto matchingFiles =
                backupDir.entryInfoList({ AUTOMATIC_BACKUP_PREFIX + rule.extensionFormat + "*.zip" }, QDir::Files | QDir::NoSymLinks, QDir::Name);

        int backupsToDelete = matchingFiles.length() - rule.maxBackupVersions;
        if (backupsToDelete > 0) {
            for (int i = 0; i < backupsToDelete; ++i) {
                auto fileInfo = matchingFiles[i].absoluteFilePath();
                QFile backupFile(fileInfo);
                if (!backupFile.remove()) {
                    qCDebug(domain_server) << "Failed to remove old backup: " << backupFile.fileName();
                }
            }
        }
    }
}

void DomainContentBackupManager::backup() {
    auto nowDateTime = QDateTime::currentDateTime();
    auto nowSeconds = nowDateTime.toSecsSinceEpoch();

    for (BackupRule& rule : _backupRules) {
        auto secondsSinceLastBackup = nowSeconds - rule.lastBackupSeconds;

        if (secondsSinceLastBackup > rule.intervalSeconds) {

            bool success;
            QString path;
            std::tie(success, path) =  createBackup(AUTOMATIC_BACKUP_PREFIX, rule.extensionFormat);
            if (!success) {
                qCWarning(domain_server) << "Failed to create backup for" << rule.name << "at" << path;
                continue;
            }

            rule.lastBackupSeconds = nowSeconds;

            removeOldBackupVersions(rule);
        }
    }
}

void DomainContentBackupManager::removeOldConsolidatedBackups() {
    constexpr std::chrono::minutes MAX_TIME_TO_KEEP_CONSOLIDATED_BACKUP { 30 };
    auto now = std::chrono::system_clock::now();
    auto it = _consolidatedBackups.begin();
    while (it != _consolidatedBackups.end()) {
        auto& backup = it->second;
        auto diff = now - backup.createdAt;
        if (diff > MAX_TIME_TO_KEEP_CONSOLIDATED_BACKUP) {
            QFile oldBackup(backup.absoluteFilePath);
            if (!oldBackup.exists() || oldBackup.remove()) {
                qDebug() << "Removed old consolidated backup: " << backup.absoluteFilePath;
                it = _consolidatedBackups.erase(it);
            } else {
                qDebug() << "Failed to remove old consolidated backup: " << backup.absoluteFilePath;
                it++;
            }
        } else {
            it++;
        }
    }
}

ConsolidatedBackupInfo DomainContentBackupManager::consolidateBackup(QString fileName) {
    {
        std::lock_guard<std::mutex> lock { _consolidatedBackupsMutex };
        auto it = _consolidatedBackups.find(fileName);

        if (it != _consolidatedBackups.end()) {
            return it->second;
        }
    }
    QMetaObject::invokeMethod(this, "consolidateBackupInternal", Q_ARG(QString, fileName));
    return {
        ConsolidatedBackupInfo::CONSOLIDATING,
        "",
        "",
        std::chrono::system_clock::now()
    };
}

void DomainContentBackupManager::consolidateBackupInternal(QString fileName) {
    auto markFailure = [this, &fileName](QString error) {
        qWarning() << "Failed to consolidate backup:" << fileName << error;
        {
            std::lock_guard<std::mutex> lock { _consolidatedBackupsMutex };
            auto& consolidatedBackup = _consolidatedBackups[fileName];
            consolidatedBackup.state = ConsolidatedBackupInfo::COMPLETE_WITH_ERROR;
            consolidatedBackup.error = error;
        }
    };

    {
        std::lock_guard<std::mutex> lock { _consolidatedBackupsMutex };

        auto it = _consolidatedBackups.find(fileName);
        if (it != _consolidatedBackups.end()) {
            return;
        }

        _consolidatedBackups[fileName] = {
            ConsolidatedBackupInfo::CONSOLIDATING,
            "",
            "",
            std::chrono::system_clock::now()
        };
    }

    QDir backupDir { _backupDirectory };
    if (!backupDir.exists()) {
        markFailure("Backup directory does not exist, bailing consolidation of backup");
        return;
    }

    auto filePath = backupDir.absoluteFilePath(fileName);
    
    if (!QFile::exists(filePath)) {
        markFailure("Backup does not exist");
        return;
    }

    auto copyFilePath = _consolidatedBackupDirectory + "/" + fileName;

    {
        QFile copyFile(copyFilePath);
        copyFile.remove();
        copyFile.close();
    }
    auto copySuccess = QFile::copy(filePath, copyFilePath);
    if (!copySuccess) {
        markFailure("Failed to create copy of backup.");
        return;
    }

    QuaZip zip(copyFilePath);
    if (!zip.open(QuaZip::mdAdd)) {
        qCritical() << "Could not open backup archive:" << filePath;
        qCritical() << "    ERROR:" << zip.getZipError();
        markFailure("Could not open backup archive");
        return;
    }

    for (auto& handler : _backupHandlers) {
        handler->consolidateBackup(fileName, zip);
    }

    zip.close();

    if (zip.getZipError() != UNZ_OK) {
        qCritical() << "Failed to consolidate backup: " << zip.getZipError();
        markFailure("Failed to consolidate backup");
        return;
    }

    {
        std::lock_guard<std::mutex> lock { _consolidatedBackupsMutex };
        auto& consolidatedBackup = _consolidatedBackups[fileName];
        consolidatedBackup.state = ConsolidatedBackupInfo::COMPLETE_WITH_SUCCESS;
        consolidatedBackup.absoluteFilePath = copyFilePath;
    }

}

void DomainContentBackupManager::createManualBackup(MiniPromise::Promise promise, const QString& name) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "createManualBackup", Q_ARG(MiniPromise::Promise, promise),
                                  Q_ARG(const QString&, name));
        return;
    }


    QRegExp nameRE { MANUAL_BACKUP_NAME_RE };
    bool success;

    if (!nameRE.exactMatch(name)) {
        qDebug() << "Cannot create manual backup with invalid name: " << name;
        success = false;
    } else {
        QString path;
        std::tie(success, path) = createBackup(MANUAL_BACKUP_PREFIX, name);
    }

    promise->resolve({
        { "success", success }
    });
}

std::pair<bool, QString> DomainContentBackupManager::createBackup(const QString& prefix, const QString& name) {
    auto timestamp = QDateTime::currentDateTime().toString(DATETIME_FORMAT);
    auto fileName = prefix + name + "-" + timestamp + ".zip";
    auto path = _backupDirectory + "/" + fileName;
    QuaZip zip(path);
    if (!zip.open(QuaZip::mdAdd)) {
        qCWarning(domain_server) << "Failed to open zip file at " << path;
        qCWarning(domain_server) << "    ERROR:" << zip.getZipError();
        return { false, path };
    }

    for (auto& handler : _backupHandlers) {
        handler->createBackup(fileName, zip);
    }

    zip.close();

    return { true, path };
}
