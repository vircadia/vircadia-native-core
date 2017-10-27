//
//  RunningMarker.cpp
//  libraries/shared/src
//
//  Created by Brad Hefta-Gaub on 2016-10-16
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RunningMarker.h"

#include <QFile>
#include <QStandardPaths>

#include "PathUtils.h"

RunningMarker::RunningMarker(QString name) :
    _name(name)
{
}

RunningMarker::~RunningMarker() {
    deleteRunningMarkerFile();
}

bool RunningMarker::fileExists() const {
    QFile runningMarkerFile(getFilePath());
    return runningMarkerFile.exists();
}

void RunningMarker::writeRunningMarkerFile() {
    QFile runningMarkerFile(getFilePath());

    // always write, even it it exists, so that it touches the files
    if (runningMarkerFile.open(QIODevice::WriteOnly)) {
        runningMarkerFile.close();
    }
}

void RunningMarker::deleteRunningMarkerFile() {
    QFile runningMarkerFile(getFilePath());
    if (runningMarkerFile.exists()) {
        runningMarkerFile.remove();
    }
}

QString RunningMarker::getFilePath() const {
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" + _name;
}
