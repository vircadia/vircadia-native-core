//
//  PathUtils.h
//
//  Created by Nissim Hadar on 11 Feb 2019.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "PathUtils.h"

#include <QFile>
#include <QProcess>
#include <QStringList>

QString PathUtils::getPathToExecutable(const QString& executableName) {
    QString path = QProcessEnvironment::systemEnvironment().value("PATH");

    QStringList pathLocations = path.replace('\\', '/').split(';');

    foreach (QString pathLocation, pathLocations) {
        if (pathLocation[pathLocation.length() - 1] != '/') {
            pathLocation += '/';
        }
        if (QFile::exists(pathLocation + executableName)) {
            return pathLocation;
        }
    }

    return QString();
}
