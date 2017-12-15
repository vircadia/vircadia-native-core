//
//  PathUtils.h
//  libraries/shared/src
//
//  Created by Brad Hefta-Gaub on 12/15/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PathUtils_h
#define hifi_PathUtils_h

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtCore/QDir>

#include "DependencyManager.h"

/**jsdoc
 * The Paths API provides absolute paths to the scripts and resources directories.
 *
 * @namespace Paths
 * @deprecated The Paths API is deprecated. Use {@link Script.resolvePath} and {@link Script.resourcesPath} instead.
 * @readonly
 * @property {string} defaultScripts - The path to the scripts directory. <em>Read-only.</em>
 * @property {string} resources - The path to the resources directory. <em>Read-only.</em>
 */
class PathUtils : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    Q_PROPERTY(QString resources READ resourcesPath CONSTANT)
    Q_PROPERTY(QUrl defaultScripts READ defaultScriptsLocation CONSTANT)
public:
    static const QString& resourcesPath();

    static QString getAppDataPath();
    static QString getAppLocalDataPath();

    static QString getAppDataFilePath(const QString& filename);
    static QString getAppLocalDataFilePath(const QString& filename);

    static QString generateTemporaryDir();

    static int removeTemporaryApplicationDirs(QString appName = QString::null);

    static Qt::CaseSensitivity getFSCaseSensitivity();
    static QString stripFilename(const QUrl& url);
    // note: this is FS-case-sensitive version of parentURL.isParentOf(childURL)
    static bool isDescendantOf(const QUrl& childURL, const QUrl& parentURL);
    static QUrl defaultScriptsLocation(const QString& newDefault = "");

};

QString fileNameWithoutExtension(const QString& fileName, const QVector<QString> possibleExtensions);
QString findMostRecentFileExtension(const QString& originalFileName, QVector<QString> possibleExtensions);

#endif // hifi_PathUtils_h
