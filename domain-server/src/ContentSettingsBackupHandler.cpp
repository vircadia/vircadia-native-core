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

static const QString DATETIME_FORMAT { "yyyy-MM-dd_HH-mm-ss" };

ContentSettingsBackupHandler::ContentSettingsBackupHandler(DomainServerSettingsManager& domainServerSettingsManager) :
    _settingsManager(domainServerSettingsManager)
{
}

static const QString CONTENT_SETTINGS_BACKUP_FILENAME = "content-settings.json";
static const QString INSTALLED_ARCHIVE = "installed_archive";
static const QString INSTALLED_ARCHIVE_FILENAME = "filename";
static const QString INSTALLED_ARCHIVE_INSTALL_TIME = "install_time";
static const QString INSTALLED_ARCHIVE_INSTALLED_BY = "installed_by";

void ContentSettingsBackupHandler::createBackup(const QString& backupName, QuaZip& zip) {

    // grab the content settings as JSON, excluding default values and values hidden from backup
    QJsonObject contentSettingsJSON = _settingsManager.settingsResponseObjectForType(
        "", // include all settings types
        DomainServerSettingsManager::Authenticated, DomainServerSettingsManager::NoDomainSettings,
        DomainServerSettingsManager::IncludeContentSettings, DomainServerSettingsManager::NoDefaultSettings,
        DomainServerSettingsManager::ForBackup
    );

    // save the filename internally as it's used to determine the name/creation time
    // of the archive, and we don't want that to change if the actual file is renamed
    // after download.
    QJsonObject installed_archive {
        { INSTALLED_ARCHIVE_FILENAME, backupName },
    };

    contentSettingsJSON.insert(INSTALLED_ARCHIVE, installed_archive);

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

std::pair<bool, QString> ContentSettingsBackupHandler::recoverBackup(const QString& backupName, QuaZip& zip) {
    if (!zip.setCurrentFile(CONTENT_SETTINGS_BACKUP_FILENAME)) {
        QString errorStr("Failed to find " + CONTENT_SETTINGS_BACKUP_FILENAME + " while recovering backup");
        qWarning() << errorStr;
        return { false, errorStr };
    }

    QuaZipFile zipFile { &zip };
    if (!zipFile.open(QIODevice::ReadOnly)) {
        QString errorStr("Failed to open " + CONTENT_SETTINGS_BACKUP_FILENAME + " in backup");
        qCritical() << errorStr;
        return { false, errorStr };
    }

    auto rawData = zipFile.readAll();
    zipFile.close();

    if (zipFile.getZipError() != UNZ_OK) {
        QString errorStr("Failed to unzip " + CONTENT_SETTINGS_BACKUP_FILENAME + ": " + zipFile.getZipError());
        qCritical() << errorStr;
        return { false, errorStr };
    }

    QJsonDocument jsonDocument = QJsonDocument::fromJson(rawData);
    QJsonObject jsonObject = jsonDocument.object();

    auto archiveJson = jsonObject.find(INSTALLED_ARCHIVE)->toObject();

    QJsonObject installed_archive {
        { INSTALLED_ARCHIVE_FILENAME, archiveJson[INSTALLED_ARCHIVE_FILENAME]},
        { INSTALLED_ARCHIVE_INSTALL_TIME, QDateTime::currentDateTime().currentMSecsSinceEpoch() },
        { INSTALLED_ARCHIVE_INSTALLED_BY, ""}
    };

    jsonObject.insert(INSTALLED_ARCHIVE, installed_archive);

    if (!_settingsManager.restoreSettingsFromObject(jsonObject, ContentSettings)) {
        QString errorStr("Failed to restore settings from " + CONTENT_SETTINGS_BACKUP_FILENAME + " in content archive");
        qCritical() << errorStr;
        return { false, errorStr };
    }
    return { true, QString() };
}
