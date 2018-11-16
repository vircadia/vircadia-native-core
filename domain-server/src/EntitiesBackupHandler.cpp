//
//  EntitiesBackupHandler.cpp
//  domain-server/src
//
//  Created by Clement Brisset on 2/14/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntitiesBackupHandler.h"

#include <QDebug>

#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#endif

#include <quazip5/quazip.h>
#include <quazip5/quazipfile.h>

#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <OctreeDataUtils.h>

EntitiesBackupHandler::EntitiesBackupHandler(QString entitiesFilePath, QString entitiesReplacementFilePath) :
    _entitiesFilePath(entitiesFilePath),
    _entitiesReplacementFilePath(entitiesReplacementFilePath)
{
}

static const QString ENTITIES_BACKUP_FILENAME = "models.json.gz";

void EntitiesBackupHandler::createBackup(const QString& backupName, QuaZip& zip) {
    QFile entitiesFile { _entitiesFilePath };

    if (entitiesFile.open(QIODevice::ReadOnly)) {
        QuaZipFile zipFile { &zip };
        if (!zipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(ENTITIES_BACKUP_FILENAME, _entitiesFilePath))) {
            qCritical().nospace() << "Failed to open " << ENTITIES_BACKUP_FILENAME << " for writing in zip";
            return;
        }
        auto entityData = entitiesFile.readAll();
        if (zipFile.write(entityData) != entityData.size()) {
            qCritical() << "Failed to write entities file to backup";
            zipFile.close();
            return;
        }
        zipFile.close();
        if (zipFile.getZipError() != UNZ_OK) {
            qCritical().nospace() << "Failed to zip " << ENTITIES_BACKUP_FILENAME << ": " << zipFile.getZipError();
        }
    }
}

void EntitiesBackupHandler::recoverBackup(const QString& backupName, QuaZip& zip) {
    if (!zip.setCurrentFile(ENTITIES_BACKUP_FILENAME)) {
        qWarning() << "Failed to find" << ENTITIES_BACKUP_FILENAME << "while recovering backup";
        return;
    }
    QuaZipFile zipFile { &zip };
    if (!zipFile.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open" << ENTITIES_BACKUP_FILENAME << "in backup";
        return;
    }
    auto rawData = zipFile.readAll();

    zipFile.close();

    OctreeUtils::RawEntityData data;
    if (!data.readOctreeDataInfoFromData(rawData)) {
        qCritical() << "Unable to parse octree data during backup recovery";
        return;
    }

    data.resetIdAndVersion();

    if (zipFile.getZipError() != UNZ_OK) {
        qCritical().nospace() << "Failed to unzip " << ENTITIES_BACKUP_FILENAME << ": " << zipFile.getZipError();
        return;
    }

    QFile entitiesFile { _entitiesReplacementFilePath };

    if (entitiesFile.open(QIODevice::WriteOnly)) {
        entitiesFile.write(data.toGzippedByteArray());
    }
}
