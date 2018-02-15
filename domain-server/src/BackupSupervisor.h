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

#include <AssetUtils.h>

#include <ReceivedMessage.h>

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

private:
    void loadAllBackups();
    bool loadBackup(const QString& backupFile);

    void startBackup() { _backupInProgress = true; }
    void finishBackup() { _backupInProgress = false; }
    void backupMissingFiles(const AssetUtils::Mappings& mappings);
    void backupNextMissingFile();
    bool writeBackupFile(const AssetUtils::AssetMappings& mappings);
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
};

#endif /* hifi_BackupSupervisor_h */
