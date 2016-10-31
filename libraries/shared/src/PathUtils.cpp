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


const QString& PathUtils::resourcesPath() {
#ifdef Q_OS_MAC
    static QString staticResourcePath = QCoreApplication::applicationDirPath() + "/../Resources/";
#else
    static QString staticResourcePath = QCoreApplication::applicationDirPath() + "/resources/";
#endif

    return staticResourcePath;
}

QString PathUtils::getRootDataDirectory() {
    auto dataPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

#ifdef Q_OS_WIN
    dataPath += "/AppData/Roaming/";
#elif defined(Q_OS_OSX)
    dataPath += "/Library/Application Support/";
#else
    dataPath += "/.local/share/";
#endif

    return dataPath;
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

QUrl defaultScriptsLocation() {
    // return "http://s3.amazonaws.com/hifi-public";
#ifdef Q_OS_WIN
    QString path = QCoreApplication::applicationDirPath() + "/scripts";
#elif defined(Q_OS_OSX)
    QString path = QCoreApplication::applicationDirPath() + "/../Resources/scripts";
#else
    QString path = QCoreApplication::applicationDirPath() + "/scripts";
#endif

    QFileInfo fileInfo(path);
    return QUrl::fromLocalFile(fileInfo.canonicalFilePath());
}
