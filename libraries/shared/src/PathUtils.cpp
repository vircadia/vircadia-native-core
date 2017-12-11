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

#include "PathUtils.h"

#include <QCoreApplication>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QtCore/QStandardPaths>
#include <QRegularExpression>
#include <mutex> // std::once
#include "shared/GlobalAppProperties.h"
#include "SharedUtil.h"

// Format: AppName-PID-Timestamp
// Example: ...
QString TEMP_DIR_FORMAT { "%1-%2-%3" };

const QString& PathUtils::resourcesPath() {
#ifdef Q_OS_MAC
    static const QString staticResourcePath = QCoreApplication::applicationDirPath() + "/../Resources/";
#else
    static const QString staticResourcePath = QCoreApplication::applicationDirPath() + "/resources/";
#endif
    return staticResourcePath;
}

#ifdef DEV_BUILD
const QString& PathUtils::projectRootPath() {
    static QString sourceFolder;
    static std::once_flag once;
    std::call_once(once, [&] {
        QDir thisDir = QFileInfo(__FILE__).absoluteDir();
        sourceFolder = QDir::cleanPath(thisDir.absoluteFilePath("../../../"));
    });
    return sourceFolder;
}
#endif

const QString& PathUtils::qmlBasePath() {
#ifdef DEV_BUILD
    static const QString staticResourcePath = QUrl::fromLocalFile(projectRootPath() + "/interface/resources/qml/").toString();
#else
    static const QString staticResourcePath = "qrc:///qml/";
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

QString PathUtils::generateTemporaryDir() {
    QDir rootTempDir = QDir::tempPath();
    QString appName = qApp->applicationName();
    for (auto i = 0; i < 64; ++i) {
        auto now = std::chrono::system_clock::now().time_since_epoch().count();
        auto dirName = TEMP_DIR_FORMAT.arg(appName).arg(qApp->applicationPid()).arg(now);
        QDir tempDir = rootTempDir.filePath(dirName);
        if (tempDir.mkpath(".")) {
            return tempDir.absolutePath();
        }
    }
    return "";
}

// Delete all temporary directories for an application
int PathUtils::removeTemporaryApplicationDirs(QString appName) {
    if (appName.isNull()) {
        appName = qApp->applicationName();
    }

    auto dirName = TEMP_DIR_FORMAT.arg(appName).arg("*").arg("*");

    QDir rootTempDir = QDir::tempPath();
    auto dirs = rootTempDir.entryInfoList({ dirName }, QDir::Dirs);
    int removed = 0;
    for (auto& dir : dirs) {
        auto dirName = dir.fileName();
        auto absoluteDirPath = QDir(dir.absoluteFilePath());
        QRegularExpression re { "^" + QRegularExpression::escape(appName) + "\\-(?<pid>\\d+)\\-(?<timestamp>\\d+)$" };

        auto match = re.match(dirName);
        if (match.hasMatch()) {
            auto pid = match.capturedRef("pid").toLongLong();
            auto timestamp = match.capturedRef("timestamp");
            if (!processIsRunning(pid)) {
                qDebug() << "  Removing old temporary directory: " << dir.absoluteFilePath();
                absoluteDirPath.removeRecursively();
                removed++;
            } else {
                qDebug() << "  Not removing (process is running): " << dir.absoluteFilePath();
            }
        }
    }

    return removed;
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
#if defined(Q_OS_OSX)
        path = QCoreApplication::applicationDirPath() + "/../Resources/scripts";
#elif defined(Q_OS_ANDROID)
        path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/scripts";
#else
        path = QCoreApplication::applicationDirPath() + "/scripts";
#endif
    }

    // turn the string into a legit QUrl
    return QUrl::fromLocalFile(QFileInfo(path).canonicalFilePath());
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
