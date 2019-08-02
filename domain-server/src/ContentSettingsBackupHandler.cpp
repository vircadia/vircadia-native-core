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
#include "DomainContentBackupManager.h"

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

void ContentSettingsBackupHandler::createBackup(const QString& backupName, QuaZip& zip) {

    // grab the content settings as JSON, excluding default values and values hidden from backup
    QJsonObject contentSettingsJSON = _settingsManager.settingsResponseObjectForType(
        "", // include all settings types
        DomainServerSettingsManager::Authenticated, DomainServerSettingsManager::NoDomainSettings,
        DomainServerSettingsManager::IncludeContentSettings, DomainServerSettingsManager::NoDefaultSettings,
        DomainServerSettingsManager::ForBackup
    );
    QString prefixFormat = "(" + QRegExp::escape(AUTOMATIC_BACKUP_PREFIX) + "|" + QRegExp::escape(MANUAL_BACKUP_PREFIX) + ")";
    QString nameFormat = "(.+)";
    QString dateTimeFormat = "(" + DATETIME_FORMAT_RE + ")";
    QRegExp backupNameFormat { prefixFormat + nameFormat + "-" + dateTimeFormat + "\\.zip" };

    QString name{ "" };
    QDateTime createdAt;

    if (backupNameFormat.exactMatch(backupName)) {
        name = backupNameFormat.cap(2);
        auto dateTime = backupNameFormat.cap(3);
        createdAt = QDateTime::fromString(dateTime, DATETIME_FORMAT);
    }

    QJsonObject installed_content {
        { INSTALLED_CONTENT_NAME, name},
        { INSTALLED_CONTENT_CREATION_TIME, createdAt.currentMSecsSinceEpoch()}
    };

    contentSettingsJSON.insert(INSTALLED_CONTENT, installed_content);

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

std::pair<bool, QString> ContentSettingsBackupHandler::recoverBackup(const QString& backupName, QuaZip& zip, const QString& username, const QString& sourceFilename) {
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

    auto archiveJson = jsonObject.find(INSTALLED_CONTENT)->toObject();

    QJsonObject installed_content {
        { INSTALLED_CONTENT_FILENAME, sourceFilename },
        { INSTALLED_CONTENT_NAME, archiveJson[INSTALLED_CONTENT_NAME].toString()},
        { INSTALLED_CONTENT_CREATION_TIME, archiveJson[INSTALLED_CONTENT_CREATION_TIME].toVariant().toLongLong() },
        { INSTALLED_CONTENT_INSTALL_TIME, QDateTime::currentDateTime().currentMSecsSinceEpoch() },
        { INSTALLED_CONTENT_INSTALLED_BY, username }
    };

    jsonObject.insert(INSTALLED_CONTENT, installed_content);

    if (!_settingsManager.restoreSettingsFromObject(jsonObject, ContentSettings)) {
        QString errorStr("Failed to restore settings from " + CONTENT_SETTINGS_BACKUP_FILENAME + " in content archive");
        qCritical() << errorStr;
        return { false, errorStr };
    }
    return { true, QString() };
}
