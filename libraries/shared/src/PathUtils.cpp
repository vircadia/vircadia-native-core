//
//  PathUtils.cpp
//  libraries/shared/src
//
//  Created by Brad Hefta-Gaub on 12/15/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QCoreApplication>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include "PathUtils.h"
#include <QtCore/QStandardPaths>
#include <mutex> // std::once
#include "shared/GlobalAppProperties.h"

const QString& PathUtils::resourcesPath() {
#ifdef Q_OS_MAC
    static QString staticResourcePath = QCoreApplication::applicationDirPath() + "/../Resources/";
#else
    static QString staticResourcePath = QCoreApplication::applicationDirPath() + "/resources/";
#endif

    return staticResourcePath;
}

QString PathUtils::getAppDataPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/";
}

QString PathUtils::getAppLocalDataPath() {
    QString overriddenPath = qApp->property(hifi::properties::APP_LOCAL_DATA_PATH).toString();
    // return overridden path if set
    if (!overriddenPath.isEmpty()) {
        return overriddenPath;
    }

    // otherwise return standard path
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/";
}

QString PathUtils::getAppDataFilePath(const QString& filename) {
    return QDir(getAppDataPath()).absoluteFilePath(filename);
}

QString PathUtils::getAppLocalDataFilePath(const QString& filename) {
    return QDir(getAppLocalDataPath()).absoluteFilePath(filename);
}

QString fileNameWithoutExtension(const QString& fileName, const QVector<QString> possibleExtensions) {
    QString fileNameLowered = fileName.toLower();
    foreach (const QString possibleExtension, possibleExtensions) {
        if (fileNameLowered.endsWith(possibleExtension.toLower())) {
            return fileName.left(fileName.count() - possibleExtension.count() - 1);
        }
    }
    return fileName;
}

QString findMostRecentFileExtension(const QString& originalFileName, QVector<QString> possibleExtensions) {
    QString sansExt = fileNameWithoutExtension(originalFileName, possibleExtensions);
    QString newestFileName = originalFileName;
    QDateTime newestTime = QDateTime::fromMSecsSinceEpoch(0);
    foreach (QString possibleExtension, possibleExtensions) {
        QString fileName = sansExt + "." + possibleExtension;
        QFileInfo fileInfo(fileName);
        if (fileInfo.exists() && fileInfo.lastModified() > newestTime) {
            newestFileName = fileName;
            newestTime = fileInfo.lastModified();
        }
    }
    return newestFileName;
}

QUrl PathUtils::defaultScriptsLocation(const QString& newDefaultPath) {
    static QString overriddenDefaultScriptsLocation = "";
    QString path;

    // set overriddenDefaultScriptLocation if it was passed in
    if (!newDefaultPath.isEmpty()) {
        overriddenDefaultScriptsLocation = newDefaultPath;
    }

    // use the overridden location if it is set
    if (!overriddenDefaultScriptsLocation.isEmpty()) {
        path = overriddenDefaultScriptsLocation;
    } else {
#ifdef Q_OS_WIN
        path = QCoreApplication::applicationDirPath() + "/scripts";
#elif defined(Q_OS_OSX)
        path = QCoreApplication::applicationDirPath() + "/../Resources/scripts";
#else
        path = QCoreApplication::applicationDirPath() + "/scripts";
#endif
    }

    // turn the string into a legit QUrl
    QFileInfo fileInfo(path);
    return QUrl::fromLocalFile(fileInfo.canonicalFilePath());
}

QString PathUtils::stripFilename(const QUrl& url) {
    // Guard against meaningless query and fragment parts.
    // Do NOT use PreferLocalFile as its behavior is unpredictable (e.g., on defaultScriptsLocation())
    return url.toString(QUrl::RemoveFilename | QUrl::RemoveQuery | QUrl::RemoveFragment);
}

Qt::CaseSensitivity PathUtils::getFSCaseSensitivity() {
    static Qt::CaseSensitivity sensitivity { Qt::CaseSensitive };
    static std::once_flag once;
    std::call_once(once, [&] {
            QString path = defaultScriptsLocation().toLocalFile();
            QFileInfo upperFI(path.toUpper());
            QFileInfo lowerFI(path.toLower());
            sensitivity = (upperFI == lowerFI) ? Qt::CaseInsensitive : Qt::CaseSensitive;
        });
    return sensitivity;
}

bool PathUtils::isDescendantOf(const QUrl& childURL, const QUrl& parentURL) {
    QString child = stripFilename(childURL);
    QString parent = stripFilename(parentURL);
    return child.startsWith(parent, PathUtils::getFSCaseSensitivity());
}
