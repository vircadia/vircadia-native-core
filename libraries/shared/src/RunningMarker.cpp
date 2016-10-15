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
#include <QThread>
#include <QTimer>

#include "NumericalConstants.h"
#include "PathUtils.h"


RunningMarker::RunningMarker(QObject* parent, QString name) :
    _parent(parent),
    _name(name)
{
}

void RunningMarker::startRunningMarker() {
    static const int RUNNING_STATE_CHECK_IN_MSECS = MSECS_PER_SECOND;

    // start the nodeThread so its event loop is running
    QThread* runningMarkerThread = new QThread(_parent);
    runningMarkerThread->setObjectName("Running Marker Thread");
    runningMarkerThread->start();

    writeRunningMarkerFiler(); // write the first file, even before timer

    QTimer* runningMarkerTimer = new QTimer(_parent);
    QObject::connect(runningMarkerTimer, &QTimer::timeout, [=](){
        writeRunningMarkerFiler();
    });
    runningMarkerTimer->start(RUNNING_STATE_CHECK_IN_MSECS);
}

RunningMarker::~RunningMarker() {
    deleteRunningMarkerFile();
}

void RunningMarker::writeRunningMarkerFiler() {
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

QString RunningMarker::getFilePath() {
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" + _name;
}

QString RunningMarker::getMarkerFilePath(QString name) {
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" + name;
}

