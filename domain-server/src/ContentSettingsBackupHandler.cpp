//
//  ContentSettingsBackupHandler.cpp
//  domain-server/src
//
//  Created by Stephen Birarda on 2/15/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ContentSettingsBackupHandler.h"

#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#endif

#include <quazip5/quazip.h>
#include <quazip5/quazipfile.h>
#include <quazip5/quazipdir.h>

#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif


ContentSettingsBackupHandler::ContentSettingsBackupHandler(DomainServerSettingsManager& domainServerSettingsManager) :
    _settingsManager(domainServerSettingsManager)
{
}

static const QString CONTENT_SETTINGS_BACKUP_FILENAME = "content-settings.json";

void ContentSettingsBackupHandler::createBackup(const QString& backupName, QuaZip& zip) {

    // grab the content settings as JSON, excluding default values and values hidden from backup
    QJsonObject contentSettingsJSON = _settingsManager.settingsResponseObjectForType(
        "", // include all settings types
        DomainServerSettingsManager::Authenticated, DomainServerSettingsManager::NoDomainSettings,
        DomainServerSettingsManager::IncludeContentSettings, DomainServerSettingsManager::NoDefaultSettings,
        DomainServerSettingsManager::ForBackup
    );

    // make a QJsonDocument using the object
    QJsonDocument contentSettingsDocument { contentSettingsJSON };

    QuaZipFile zipFile { &zip };

    if (zipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(CONTENT_SETTINGS_BACKUP_FILENAME))) {
        if (zipFile.write(contentSettingsDocument.toJson()) == -1) {
            qCritical().nospace() << "Failed to write to " << CONTENT_SETTINGS_BACKUP_FILENAME << ": " << zipFile.getZipError();
        }

        zipFile.close();

        if (zipFile.getZipError() != UNZ_OK) {
            qCritical().nospace() << "Failed to zip " << CONTENT_SETTINGS_BACKUP_FILENAME << ": " << zipFile.getZipError();
        }
    } else {
        qCritical().nospace() << "Failed to open " << CONTENT_SETTINGS_BACKUP_FILENAME << ": " << zipFile.getZipError();
    }
}

void ContentSettingsBackupHandler::recoverBackup(const QString& backupName, QuaZip& zip) {
    if (!zip.setCurrentFile(CONTENT_SETTINGS_BACKUP_FILENAME)) {
        qWarning() << "Failed to find" << CONTENT_SETTINGS_BACKUP_FILENAME << "while recovering backup";
        return;
    }

    QuaZipFile zipFile { &zip };
    if (!zipFile.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open" << CONTENT_SETTINGS_BACKUP_FILENAME << "in backup";
        return;
    }

    auto rawData = zipFile.readAll();
    zipFile.close();

    QJsonDocument jsonDocument = QJsonDocument::fromJson(rawData);

    if (!_settingsManager.restoreSettingsFromObject(jsonDocument.object(), ContentSettings)) {
        qCritical() << "Failed to restore settings from" << CONTENT_SETTINGS_BACKUP_FILENAME << "in content archive";
    }
}
