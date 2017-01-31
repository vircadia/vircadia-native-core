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

#include "DependencyManager.h"

/**jsdoc
 * @namespace Paths
 * @readonly
 * @property {string} resources The path to the resources directory.
 */
class PathUtils : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    Q_PROPERTY(QString resources READ resourcesPath)
public:
    static const QString& resourcesPath();
    static QString getRootDataDirectory();
};

QString fileNameWithoutExtension(const QString& fileName, const QVector<QString> possibleExtensions);
QString findMostRecentFileExtension(const QString& originalFileName, QVector<QString> possibleExtensions);

QUrl defaultScriptsLocation();

#endif // hifi_PathUtils_h
