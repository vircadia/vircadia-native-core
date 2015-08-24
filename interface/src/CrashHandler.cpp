//
//  CrashHandler.cpp
//  interface/src
//
//  Created by David Rowe on 24 Aug 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CrashHandler.h"

#include <QCoreApplication>
#include <QFile>
#include <PathUtils.h>
#include <QSettings>
#include <QStandardPaths>

static const QString RUNNING_MARKER_FILENAME = "Interface.running";

void CrashHandler::writeRunningMarkerFiler() {
    QFile runningMarkerFile(runningMarkerFilePath());
    if (!runningMarkerFile.exists()) {
        runningMarkerFile.open(QIODevice::WriteOnly);
        runningMarkerFile.close();
    }
}
void CrashHandler::deleteRunningMarkerFile() {
    QFile runningMarkerFile(runningMarkerFilePath());
    if (runningMarkerFile.exists()) {
        runningMarkerFile.remove();
    }
}

const QString CrashHandler::runningMarkerFilePath() {
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings applicationInfo(PathUtils::resourcesPath() + "info/ApplicationInfo.ini", QSettings::IniFormat);
    applicationInfo.beginGroup("INFO");
    QCoreApplication::setOrganizationName(applicationInfo.value("organizationName").toString());
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" + RUNNING_MARKER_FILENAME;
}
