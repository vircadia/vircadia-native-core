//
//  AssetsBackupHandler.h
//  domain-server/src
//
//  Created by Clement Brisset on 1/12/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AssetsBackupHandler_h
#define hifi_AssetsBackupHandler_h

#include <set>
#include <map>

#include <QObject>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>

#include <AssetUtils.h>
#include <ReceivedMessage.h>
#include <PortableHighResolutionClock.h>

#include "BackupHandler.h"

class AssetsBackupHandler : public QObject, public BackupHandlerInterface {
    Q_OBJECT

public:
    AssetsBackupHandler(const QString& backupDirectory, bool assetServerEnabled);

    std::pair<bool, float> isAvailable(const QString& backupName) override;
    std::pair<bool, float> getRecoveryStatus() override;

    void loadBackup(const QString& backupName, QuaZip& zip) override;
    void loadingComplete() override;
    void createBackup(const QString& backupName, QuaZip& zip) override;
    std::pair<bool, QString> recoverBackup(const QString& backupName, QuaZip& zip, const QString& username, const QString& sourceFilename) override;
    void deleteBackup(const QString& backupName) override;
    void consolidateBackup(const QString& backupName, QuaZip& zip) override;
    bool isCorruptedBackup(const QString& backupName) override;

    bool operationInProgress() { return getRecoveryStatus().first; }

private:
    void setupRefreshTimer();
    void refreshMappings();

    void refreshAssetsInBackups();
    void refreshAssetsOnDisk();
    void checkForMissingAssets();
    void checkForAssetsToDelete();

    void downloadMissingFiles(const AssetUtils::Mappings& mappings);
    void downloadNextMissingFile();
    bool writeAssetFile(const AssetUtils::AssetHash& hash, const QByteArray& data);

    void computeServerStateDifference(const AssetUtils::Mappings& currentMappings,
                                      const AssetUtils::Mappings& newMappings);
    void restoreAllAssets();
    void restoreNextAsset();
    void updateMappings();

    QString _assetsDirectory;
    bool _assetServerEnabled { false };

    QTimer _mappingsRefreshTimer;
    p_high_resolution_clock::time_point _lastMappingsRefresh;
    AssetUtils::Mappings _currentMappings;

    struct AssetServerBackup {
        AssetServerBackup(const QString& pName, AssetUtils::Mappings pMappings, bool pCorruptedBackup) :
            name(pName), mappings(pMappings), corruptedBackup(pCorruptedBackup) {}

        QString name;
        AssetUtils::Mappings mappings;
        bool corruptedBackup;
    };

    // Internal storage for backups on disk
    std::vector<AssetServerBackup> _backups;
    std::set<AssetUtils::AssetHash> _assetsInBackups;
    std::set<AssetUtils::AssetHash> _assetsOnDisk;

    // Internal storage for backup in progress
    std::set<AssetUtils::AssetHash> _assetsLeftToRequest;

    // Internal storage for restore in progress
    std::vector<AssetUtils::AssetHash> _assetsLeftToUpload;
    std::vector<std::pair<AssetUtils::AssetPath, AssetUtils::AssetHash>> _mappingsLeftToSet;
    AssetUtils::AssetPathList _mappingsLeftToDelete;
    int _mappingRequestsInFlight { 0 };
    int _numRestoreOperations { 0 }; // Used to compute a restore progress.
};

#endif /* hifi_AssetsBackupHandler_h */
