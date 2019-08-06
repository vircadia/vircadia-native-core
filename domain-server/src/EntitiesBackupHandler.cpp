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

std::pair<bool, QString> EntitiesBackupHandler::recoverBackup(const QString& backupName, QuaZip& zip, const QString& username, const QString& sourceFilename) {
    if (!zip.setCurrentFile(ENTITIES_BACKUP_FILENAME)) {
        QString errorStr("Failed to find " + ENTITIES_BACKUP_FILENAME + " while recovering backup");
        qWarning() << errorStr;
        return { false, errorStr };
    }
    QuaZipFile zipFile { &zip };
    if (!zipFile.open(QIODevice::ReadOnly)) {
        QString errorStr("Failed to open " + ENTITIES_BACKUP_FILENAME + " in backup");
        qCritical() << errorStr;
        return { false, errorStr };
    }
    auto rawData = zipFile.readAll();

    zipFile.close();

    if (zipFile.getZipError() != UNZ_OK) {
        QString errorStr("Failed to unzip " + ENTITIES_BACKUP_FILENAME + ": " + zipFile.getZipError());
        qCritical() << errorStr;
        return { false, errorStr };
    }

    OctreeUtils::RawEntityData data;
    if (!data.readOctreeDataInfoFromData(rawData)) {
        QString errorStr("Unable to parse octree data during backup recovery");
        qCritical() << errorStr;
        return { false, errorStr };
    }

    data.resetIdAndVersion();

    QFile entitiesFile { _entitiesReplacementFilePath };

    if (entitiesFile.open(QIODevice::WriteOnly)) {
        entitiesFile.write(data.toGzippedByteArray());
    }
    return { true, QString() };
}
