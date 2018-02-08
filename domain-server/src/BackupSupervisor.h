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

#include <ReceivedMessage.h>

class QuaZip;

struct AssetServerBackup {
    QString filePath;
    AssetUtils::Mappings mappings;
    bool corruptedBackup;
};

class BackupSupervisor : public QObject {
    Q_OBJECT

public:
    BackupSupervisor(const QString& backupDirectory);

    void loadBackup(QuaZip& zip);
    void createBackup(QuaZip& zip);
    void recoverBackup(QuaZip& zip);
    void deleteBackup(QuaZip& zip);
    void consolidateBackup(QuaZip& zip);

    bool operationInProgress() const { return _operationInProgress; }

private:
    void refreshMappings();

    void refreshAssetsInBackups();
    void refreshAssetsOnDisk();
    void checkForMissingAssets();
    void checkForAssetsToDelete();

    void startOperation() { _operationInProgress = true; }
    void stopOperation() { _operationInProgress = false; }

    void downloadMissingFiles(const AssetUtils::Mappings& mappings);
    void downloadNextMissingFile();
    bool writeAssetFile(const AssetUtils::AssetHash& hash, const QByteArray& data);

    void computeServerStateDifference(const AssetUtils::Mappings& currentMappings,
                                      const AssetUtils::Mappings& newMappings);
    void restoreAllAssets();
    void restoreNextAsset();
    void updateMappings();

    QString _assetsDirectory;

    QTimer _mappingsRefreshTimer;
    quint64 _lastMappingsRefresh { 0 };
    AssetUtils::Mappings _currentMappings;

    bool _operationInProgress { false };

    // Internal storage for backups on disk
    bool _allBackupsLoadedSuccessfully { false };
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
};


#include <quazip5/quazipfile.h>
class AssetsBackupHandler {
public:
    AssetsBackupHandler(BackupSupervisor* backupSupervisor) : _backupSupervisor(backupSupervisor) {}

    void loadBackup(QuaZip& zip) {}

    void createBackup(QuaZip& zip) {
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

    void recoverBackup(QuaZip& zip) {}
    void deleteBackup(QuaZip& zip) {}
    void consolidateBackup(QuaZip& zip) {}

private:
    BackupSupervisor* _backupSupervisor;
};

#endif /* hifi_BackupSupervisor_h */
