//
//  BackupSupervisor.h
//  domain-server/src
//
//  Created by Clement Brisset on 1/12/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BackupSupervisor_h
#define hifi_BackupSupervisor_h

#include <set>
#include <map>

#include <QObject>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>

#include <AssetUtils.h>
#include <SharedUtil.h>

#include <ReceivedMessage.h>

class QuaZip;

struct AssetServerBackup {
    std::string filePath;
    AssetUtils::Mappings mappings;
    bool corruptedBackup;
};

class BackupSupervisor : public QObject {
    Q_OBJECT

public:
    BackupSupervisor();

    void backupAssetServer();
    void restoreAssetServer(int backupIndex);
    bool deleteBackup(int backupIndex);

    const std::vector<AssetServerBackup>& getBackups() const { return _backups; };

    bool backupInProgress() const { return _backupInProgress; }
    bool restoreInProgress() const { return _restoreInProgress; }

    AssetUtils::Mappings getCurrentMappings() const { return _currentMappings; }
    quint64 getLastRefreshTimestamp() const { return _lastMappingsRefresh; }

private:
    void refreshMappings();

    void loadAllBackups();
    bool loadBackup(const QString& backupFile);

    void startBackup() { _backupInProgress = true; }
    void finishBackup() { _backupInProgress = false; }
    void backupMissingFiles(const AssetUtils::Mappings& mappings);
    void backupNextMissingFile();
    bool writeBackupFile(const AssetUtils::Mappings& mappings);
    bool writeAssetFile(const AssetUtils::AssetHash& hash, const QByteArray& data);

    void startRestore() { _restoreInProgress = true; }
    void finishRestore() { _restoreInProgress = false; }
    void computeServerStateDifference(const AssetUtils::AssetMappings& currentMappings,
                                      const AssetUtils::Mappings& newMappings);
    void restoreAllAssets();
    void restoreNextAsset();
    void updateMappings();

    QString _backupsDirectory;
    QString _assetsDirectory;


    quint64 _lastMappingsRefresh { 0 };
    AssetUtils::Mappings _currentMappings;

    // Internal storage for backups on disk
    bool _allBackupsLoadedSuccessfully { false };
    std::vector<AssetServerBackup> _backups;
    std::set<AssetUtils::AssetHash> _assetsInBackups;
    std::set<AssetUtils::AssetHash> _assetsOnDisk;

    // Internal storage for backup in progress
    bool _backupInProgress { false };
    std::vector<AssetUtils::AssetHash> _assetsLeftToRequest;

    // Internal storage for restore in progress
    bool _restoreInProgress { false };
    std::vector<AssetUtils::AssetHash> _assetsLeftToUpload;
    std::vector<std::pair<AssetUtils::AssetPath, AssetUtils::AssetHash>> _mappingsLeftToSet;
    AssetUtils::AssetPathList _mappingsLeftToDelete;
    int _mappingRequestsInFlight { 0 };

    QTimer _mappingsRefreshTimer;
};


#include <quazip5/quazipfile.h>
class AssetsBackupHandler {
public:
    AssetsBackupHandler(BackupSupervisor* backupSupervisor) : _backupSupervisor(backupSupervisor) {}

    void loadBackup(const QuaZip& zip) {}

    void createBackup(QuaZip& zip) const {
        quint64 lastRefreshTimestamp = _backupSupervisor->getLastRefreshTimestamp();
        AssetUtils::Mappings mappings = _backupSupervisor->getCurrentMappings();

        if (lastRefreshTimestamp == 0) {
            qWarning() << "Current mappings not yet loaded, ";
            return;
        }

        static constexpr quint64 MAX_REFRESH_TIME = 15 * 60 * 1000 * 1000;
        if (usecTimestampNow() - lastRefreshTimestamp > MAX_REFRESH_TIME) {
            qWarning() << "Backing up asset mappings that appear old.";
        }

        QJsonObject jsonObject;
        for (const auto& mapping : mappings) {
            jsonObject.insert(mapping.first, mapping.second);
        }
        QJsonDocument document(jsonObject);

        QuaZipFile zipFile { &zip };
        if (!zipFile.open(QIODevice::WriteOnly, QuaZipNewInfo("mappings.json"))) {
            qDebug() << "testCreate(): outFile.open()";
        }
        zipFile.write(document.toJson());
        zipFile.close();
        if (zipFile.getZipError() != UNZ_OK) {
            qDebug() << "testCreate(): outFile.close(): " << zipFile.getZipError();
        }
    }

    void recoverBackup(const QuaZip& zip) const {}
    void deleteBackup(const QuaZip& zip) {}
    void consolidateBackup(QuaZip& zip) const {}

private:
    BackupSupervisor* _backupSupervisor;
};

#endif /* hifi_BackupSupervisor_h */
