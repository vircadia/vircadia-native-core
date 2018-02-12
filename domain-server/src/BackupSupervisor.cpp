//
//  BackupSupervisor.cpp
//  domain-server/src
//
//  Created by Clement Brisset on 1/12/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BackupSupervisor.h"

#include <QJsonDocument>
#include <QDate>

#include <AssetClient.h>
#include <AssetRequest.h>
#include <AssetUpload.h>
#include <MappingRequest.h>
#include <PathUtils.h>

const QString BACKUPS_DIR = "backups/";
const QString ASSETS_DIR = "files/";
const QString MAPPINGS_PREFIX = "mappings-";

using namespace std;

BackupSupervisor::BackupSupervisor() {
    _backupsDirectory = PathUtils::getAppDataPath() + BACKUPS_DIR;
    QDir backupDir { _backupsDirectory };
    if (!backupDir.exists()) {
        backupDir.mkpath(".");
    }

    _assetsDirectory = PathUtils::getAppDataPath() + BACKUPS_DIR + ASSETS_DIR;
    QDir assetsDir { _assetsDirectory };
    if (!assetsDir.exists()) {
        assetsDir.mkpath(".");
    }

    loadAllBackups();
}

void BackupSupervisor::loadAllBackups() {
    _backups.clear();
    _assetsInBackups.clear();
    _assetsOnDisk.clear();
    _allBackupsLoadedSuccessfully = true;

    QDir assetsDir { _assetsDirectory };
    auto assetNames = assetsDir.entryList(QDir::Files);
    qDebug() << "Loading" << assetNames.size() << "assets.";

    // store all valid hashes
    copy_if(begin(assetNames), end(assetNames),
            inserter(_assetsOnDisk, begin(_assetsOnDisk)), AssetUtils::isValidHash);

    QDir backupsDir { _backupsDirectory };
    auto files = backupsDir.entryList({ MAPPINGS_PREFIX + "*.json" }, QDir::Files);
    qDebug() << "Loading" << files.size() << "backups.";

    for (const auto& fileName : files) {
        auto filePath = backupsDir.filePath(fileName);
        auto success = loadBackup(filePath);
        if (!success) {
            qCritical() << "Failed to load backup file" << filePath;
            _allBackupsLoadedSuccessfully = false;
        }
    }

    vector<AssetUtils::AssetHash> missingAssets;
    set_difference(begin(_assetsInBackups), end(_assetsInBackups),
                   begin(_assetsOnDisk), end(_assetsOnDisk),
                   back_inserter(missingAssets));
    if (missingAssets.size() > 0) {
        qWarning() << "Found" << missingAssets.size() << "assets missing.";
    }

    vector<AssetUtils::AssetHash> deprecatedAssets;
    set_difference(begin(_assetsOnDisk), end(_assetsOnDisk),
                   begin(_assetsInBackups), end(_assetsInBackups),
                   back_inserter(deprecatedAssets));

    if (deprecatedAssets.size() > 0) {
        qDebug() << "Found" << deprecatedAssets.size() << "assets to delete.";
        if (_allBackupsLoadedSuccessfully) {
            for (const auto& hash : deprecatedAssets) {
                QFile::remove(_assetsDirectory + hash);
            }
        } else {
            qWarning() << "Some backups did not load properly, aborting deleting for safety.";
        }
    }
}

bool BackupSupervisor::loadBackup(const QString& backupFile) {
    _backups.push_back({ backupFile.toStdString(), {}, false });
    auto& backup = _backups.back();

    QFile file { backupFile };
    if (!file.open(QFile::ReadOnly)) {
        qCritical() << "Could not open backup file:" << backupFile;
        backup.corruptedBackup = true;
        return false;
    }
    QJsonParseError error;
    auto document = QJsonDocument::fromJson(file.readAll(), &error);
    if (document.isNull() || !document.isObject()) {
        qCritical() << "Could not parse backup file to JSON object:" << backupFile;
        qCritical() << "    Error:" << error.errorString();
        backup.corruptedBackup = true;
        return false;
    }

    auto jsonObject = document.object();
    for (auto it = begin(jsonObject); it != end(jsonObject); ++it) {
        const auto& assetPath = it.key();
        const auto& assetHash = it.value().toString();

        if (!AssetUtils::isValidHash(assetHash)) {
            qCritical() << "Corrupted mapping in backup file" << backupFile << ":" << it.key();
            backup.corruptedBackup = true;
            return false;
        }

        backup.mappings[assetPath] = assetHash;
        _assetsInBackups.insert(assetHash);
    }

    _backups.push_back(backup);
    return true;
}

void BackupSupervisor::backupAssetServer() {
    if (backupInProgress() || restoreInProgress()) {
        qWarning() << "There is already a backup/restore in progress.";
        return;
    }

    auto assetClient = DependencyManager::get<AssetClient>();
    auto request = assetClient->createGetAllMappingsRequest();

    connect(request, &GetAllMappingsRequest::finished, this, [this](GetAllMappingsRequest* request) {
        qDebug() << "Got" << request->getMappings().size() << "mappings!";

        if (request->getError() != MappingRequest::NoError) {
            qCritical() << "Could not complete backup.";
            qCritical() << "    Error:" << request->getErrorString();
            finishBackup();
            request->deleteLater();
            return;
        }

        if (!writeBackupFile(request->getMappings())) {
            finishBackup();
            request->deleteLater();
            return;
        }

        assert(!_backups.empty());
        const auto& mappings = _backups.back().mappings;
        backupMissingFiles(mappings);

        request->deleteLater();
    });

    startBackup();
    request->start();
}

void BackupSupervisor::backupMissingFiles(const AssetUtils::Mappings& mappings) {
    _assetsLeftToRequest.reserve(mappings.size());
    for (auto& mapping : mappings) {
        const auto& hash = mapping.second;
        if (_assetsOnDisk.find(hash) == end(_assetsOnDisk)) {
            _assetsLeftToRequest.push_back(hash);
        }
    }

    backupNextMissingFile();
}

void BackupSupervisor::backupNextMissingFile() {
    if (_assetsLeftToRequest.empty()) {
        finishBackup();
        return;
    }

    auto hash = _assetsLeftToRequest.back();
    _assetsLeftToRequest.pop_back();

    auto assetClient = DependencyManager::get<AssetClient>();
    auto assetRequest = assetClient->createRequest(hash);

    connect(assetRequest, &AssetRequest::finished, this, [this](AssetRequest* request) {
        if (request->getError() == AssetRequest::NoError) {
            qDebug() << "Got" << request->getHash();

            bool success = writeAssetFile(request->getHash(), request->getData());
            if (!success) {
                qCritical() << "Failed to write asset file" << request->getHash();
            }
        } else {
            qCritical() << "Failed to backup asset" << request->getHash();
        }

        backupNextMissingFile();

        request->deleteLater();
    });

    assetRequest->start();
}

bool BackupSupervisor::writeBackupFile(const AssetUtils::AssetMappings& mappings) {
    auto filename = MAPPINGS_PREFIX + QDateTime::currentDateTimeUtc().toString(Qt::ISODate) + ".json";
    QFile file { PathUtils::getAppDataPath() + BACKUPS_DIR + filename };
    if (!file.open(QFile::WriteOnly)) {
        qCritical() << "Could not open backup file" << file.fileName();
        return false;
    }

    AssetServerBackup backup;
    QJsonObject jsonObject;
    for (auto& mapping : mappings) {
        backup.mappings[mapping.first] = mapping.second.hash;
        _assetsInBackups.insert(mapping.second.hash);
        jsonObject.insert(mapping.first, mapping.second.hash);
    }

    QJsonDocument document(jsonObject);
    file.write(document.toJson());

    backup.filePath = file.fileName().toStdString();
    _backups.push_back(backup);

    return true;
}

bool BackupSupervisor::writeAssetFile(const AssetUtils::AssetHash& hash, const QByteArray& data) {
    QDir assetsDir { _assetsDirectory };
    QFile file { assetsDir.filePath(hash) };
    if (!file.open(QFile::WriteOnly)) {
        qCritical() << "Could not open backup file" << file.fileName();
        return false;
    }

    file.write(data);

    _assetsOnDisk.insert(hash);

    return true;
}

void BackupSupervisor::restoreAssetServer(int backupIndex) {
    if (backupInProgress() || restoreInProgress()) {
        qWarning() << "There is already a backup/restore in progress.";
        return;
    }

    auto assetClient = DependencyManager::get<AssetClient>();
    auto request = assetClient->createGetAllMappingsRequest();

    connect(request, &GetAllMappingsRequest::finished, this, [this, backupIndex](GetAllMappingsRequest* request) {
        if (request->getError() == MappingRequest::NoError) {
            const auto& newMappings = _backups.at(backupIndex).mappings;
            computeServerStateDifference(request->getMappings(), newMappings);

            restoreAllAssets();
        } else {
            finishRestore();
        }

        request->deleteLater();
    });

    startRestore();
    request->start();
}

void BackupSupervisor::computeServerStateDifference(const AssetUtils::AssetMappings& currentMappings,
                                                    const AssetUtils::Mappings& newMappings) {
    _mappingsLeftToSet.reserve((int)newMappings.size());
    _assetsLeftToUpload.reserve((int)newMappings.size());
    _mappingsLeftToDelete.reserve((int)currentMappings.size());

    set<AssetUtils::AssetHash> currentAssets;
    for (const auto& currentMapping : currentMappings) {
        const auto& currentPath = currentMapping.first;
        const auto& currentHash = currentMapping.second.hash;

        if (newMappings.find(currentPath) == end(newMappings)) {
            _mappingsLeftToDelete.push_back(currentPath);
        }
        currentAssets.insert(currentHash);
    }

    for (const auto& newMapping : newMappings) {
        const auto& newPath = newMapping.first;
        const auto& newHash = newMapping.second;

        auto it = currentMappings.find(newPath);
        if (it == end(currentMappings) || it->second.hash != newHash) {
            _mappingsLeftToSet.push_back({ newPath, newHash });
        }
        if (currentAssets.find(newHash) == end(currentAssets)) {
            _assetsLeftToUpload.push_back(newHash);
        }
    }

    qDebug() << "Mappings to set:" << _mappingsLeftToSet.size();
    qDebug() << "Mappings to del:" << _mappingsLeftToDelete.size();
    qDebug() << "Assets to upload:" << _assetsLeftToUpload.size();
}

void BackupSupervisor::restoreAllAssets() {
    restoreNextAsset();
}

void BackupSupervisor::restoreNextAsset() {
    if (_assetsLeftToUpload.empty()) {
        updateMappings();
        return;
    }

    auto hash = _assetsLeftToUpload.back();
    _assetsLeftToUpload.pop_back();

    auto assetFilename = _assetsDirectory + hash;

    auto assetClient = DependencyManager::get<AssetClient>();
    auto request = assetClient->createUpload(assetFilename);

    connect(request, &AssetUpload::finished, this, [this](AssetUpload* request) {
        if (request->getError() != AssetUpload::NoError) {
            qCritical() << "Failed to restore asset:" << request->getFilename();
            qCritical() << "    Error:" << request->getErrorString();
        }

        restoreNextAsset();

        request->deleteLater();
    });

    request->start();
}

void BackupSupervisor::updateMappings() {
    auto assetClient = DependencyManager::get<AssetClient>();
    for (const auto& mapping : _mappingsLeftToSet) {
        auto request = assetClient->createSetMappingRequest(mapping.first, mapping.second);
        connect(request, &SetMappingRequest::finished, this, [this](SetMappingRequest* request) {
            if (request->getError() != MappingRequest::NoError) {
                qCritical() << "Failed to set mapping:" << request->getPath();
                qCritical() << "    Error:" << request->getErrorString();
            }

            if (--_mappingRequestsInFlight == 0) {
                finishRestore();
            }

            request->deleteLater();
        });

        request->start();
        ++_mappingRequestsInFlight;
    }
    _mappingsLeftToSet.clear();

    auto request = assetClient->createDeleteMappingsRequest(_mappingsLeftToDelete);
    connect(request, &DeleteMappingsRequest::finished, this, [this](DeleteMappingsRequest* request) {
        if (request->getError() != MappingRequest::NoError) {
            qCritical() << "Failed to delete mappings";
            qCritical() << "    Error:" << request->getErrorString();
        }

        if (--_mappingRequestsInFlight == 0) {
            finishRestore();
        }

        request->deleteLater();
    });
    _mappingsLeftToDelete.clear();

    request->start();
    ++_mappingRequestsInFlight;
}
bool BackupSupervisor::deleteBackup(int backupIndex) {
    if (backupInProgress() || restoreInProgress()) {
        qWarning() << "There is a backup/restore in progress.";
        return false;
    }
    const auto& filePath = _backups.at(backupIndex).filePath;
    auto success = QFile::remove(filePath.c_str());

    loadAllBackups();

    return success;
}
