//
//  BackupHandler.h
//  domain-server/src
//
//  Created by Clement Brisset on 2/5/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BackupHandler_h
#define hifi_BackupHandler_h

#include <memory>

#include <QDebug>

#include <quazip5/quazip.h>

class BackupHandlerInterface {
public:
    virtual ~BackupHandlerInterface() = default;

    virtual void loadBackup(QuaZip& zip) = 0;
    virtual void createBackup(QuaZip& zip) = 0;
    virtual void recoverBackup(QuaZip& zip) = 0;
    virtual void deleteBackup(QuaZip& zip) = 0;
    virtual void consolidateBackup(QuaZip& zip) = 0;
};
using BackupHandlerPointer = std::unique_ptr<BackupHandlerInterface>;

#include <quazip5/quazipfile.h>
#include <OctreeUtils.h>

class EntitiesBackupHandler : public BackupHandlerInterface {
public:
    EntitiesBackupHandler(QString entitiesFilePath, QString entitiesReplacementFilePath) :
        _entitiesFilePath(entitiesFilePath),
        _entitiesReplacementFilePath(entitiesReplacementFilePath) {}

    void loadBackup(QuaZip& zip) {}

    // Create a skeleton backup
    void createBackup(QuaZip& zip) {
        QFile entitiesFile { _entitiesFilePath };

        if (entitiesFile.open(QIODevice::ReadOnly)) {
            QuaZipFile zipFile { &zip };
            zipFile.open(QIODevice::WriteOnly, QuaZipNewInfo("models.json.gz", _entitiesFilePath));
            zipFile.write(entitiesFile.readAll());
            zipFile.close();
            if (zipFile.getZipError() != UNZ_OK) {
                qCritical() << "Failed to zip models.json.gz: " << zipFile.getZipError();
            }
        }
    }

    // Recover from a full backup
    void recoverBackup(QuaZip& zip) {
        if (!zip.setCurrentFile("models.json.gz")) {
            qWarning() << "Failed to find models.json.gz while recovering backup";
            return;
        }
        QuaZipFile zipFile { &zip };
        if (!zipFile.open(QIODevice::ReadOnly)) {
            qCritical() << "Failed to open models.json.gz in backup";
            return;
        }
        auto rawData = zipFile.readAll();

        zipFile.close();

        OctreeUtils::RawOctreeData data;
        if (!OctreeUtils::readOctreeDataInfoFromData(rawData, &data)) {
            qCritical() << "Unable to parse octree data during backup recovery";
            return;
        }

        data.resetIdAndVersion();

        if (zipFile.getZipError() != UNZ_OK) {
            qCritical() << "Failed to unzip models.json.gz: " << zipFile.getZipError();
            return;
        }

        QFile entitiesFile { _entitiesReplacementFilePath };

        if (entitiesFile.open(QIODevice::WriteOnly)) {
            entitiesFile.write(data.toGzippedByteArray());
        }
    }

    // Delete a skeleton backup
    void deleteBackup(QuaZip& zip) {
    }

    // Create a full backup
    void consolidateBackup(QuaZip& zip) {
    }

private:
    QString _entitiesFilePath;
    QString _entitiesReplacementFilePath;
};

#endif /* hifi_BackupHandler_h */
