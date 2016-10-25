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
    qDebug() << __FUNCTION__ << "parent:" << parent << "name:" << name;
}

void RunningMarker::startRunningMarker() {
    static const int RUNNING_STATE_CHECK_IN_MSECS = MSECS_PER_SECOND;

    // start the nodeThread so its event loop is running
    _runningMarkerThread = new QThread(_parent);
    _runningMarkerThread->setObjectName("Running Marker Thread");
    _runningMarkerThread->start();

    writeRunningMarkerFiler(); // write the first file, even before timer

    _runningMarkerTimer = new QTimer();
    QObject::connect(_runningMarkerTimer, &QTimer::timeout, [=](){
        writeRunningMarkerFiler();
    });
    //QObject::connect(_runningMarkerThread, &QThread::finished, _runningMarkerTimer, &QObject::deleteLater);
    _runningMarkerTimer->start(RUNNING_STATE_CHECK_IN_MSECS);

    // put the time on the thread
    _runningMarkerTimer->moveToThread(_runningMarkerThread);
}

RunningMarker::~RunningMarker() {
    deleteRunningMarkerFile();
    _runningMarkerTimer->stop();
    _runningMarkerThread->quit();
    _runningMarkerTimer->deleteLater();
    _runningMarkerThread->deleteLater();
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

