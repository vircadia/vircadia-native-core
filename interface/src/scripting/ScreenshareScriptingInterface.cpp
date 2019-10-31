//
//  ScreenshareScriptingInterface.cpp
//  interface/src/scripting/
//
//  Created by Milad Nazeri on 2019-10-23.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScreenshareScriptingInterface.h"
#include <QThread>
#include <QDesktopServices>
#include <QUrl>
#include <QCoreApplication>


ScreenshareScriptingInterface::ScreenshareScriptingInterface() {
};

ScreenshareScriptingInterface::~ScreenshareScriptingInterface() {
    stopScreenshare();
}

void ScreenshareScriptingInterface::startScreenshare(QString displayName, QString userName, QString token, QString sessionID, QString apiKey) {
    if (QThread::currentThread() != thread()) {
        // We must start a new QProcess from the main thread.
        QMetaObject::invokeMethod(
            this, "startScreenshare", 
            Q_ARG(QString, displayName),
            Q_ARG(QString, userName),
            Q_ARG(QString, token),
            Q_ARG(QString, sessionID),
            Q_ARG(QString, apiKey)
        );
        return;
    }

    if (_screenshareProcess && _screenshareProcess->state() != QProcess::NotRunning) {
        qDebug() << "Screenshare process already running. Aborting...";
        return;
    }

    _screenshareProcess.reset(new QProcess(this));

    QFileInfo screenshareExecutable(SCREENSHARE_EXE_PATH);
    if (!screenshareExecutable.exists() || !screenshareExecutable.isFile()) {
        qDebug() << "Screenshare executable doesn't exist at" << SCREENSHARE_EXE_PATH;
        return;
    }

    if (displayName.isEmpty() || userName.isEmpty() || token.isEmpty() || sessionID.isEmpty() || apiKey.isEmpty()) {
        qDebug() << "Screenshare executable can't launch without connection info.";
        return;
    }

    QStringList arguments;
    arguments << "--userName=" + userName;
    arguments << "--displayName=" + displayName; 
    arguments << "--token=" + token; 
    arguments << "--apiKey=" + apiKey; 
    arguments << "--sessionID=" + sessionID; 

    connect(_screenshareProcess.get(), &QProcess::errorOccurred,
        [=](QProcess::ProcessError error) { qDebug() << "ZRF QProcess::errorOccurred. `error`:" << error; });
    connect(_screenshareProcess.get(), &QProcess::started, [=]() { qDebug() << "ZRF QProcess::started"; });
    connect(_screenshareProcess.get(), &QProcess::stateChanged,
        [=](QProcess::ProcessState newState) { qDebug() << "ZRF QProcess::stateChanged. `newState`:" << newState; });
    connect(_screenshareProcess.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [=](int exitCode, QProcess::ExitStatus exitStatus) {
            qDebug() << "ZRF QProcess::finished. `exitCode`:" << exitCode << "`exitStatus`:" << exitStatus;
            emit screenshareStopped();
        });

    _screenshareProcess->start(SCREENSHARE_EXE_PATH, arguments);
};

void ScreenshareScriptingInterface::stopScreenshare() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "stopScreenshare");
        return;
    }

    if (_screenshareProcess && _screenshareProcess->state() != QProcess::NotRunning) {
        _screenshareProcess->terminate();
    }
}
