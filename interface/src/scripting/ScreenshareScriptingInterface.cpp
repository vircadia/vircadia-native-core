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

#include <QCoreApplication>
#include <QDesktopServices>
#include <QThread>
#include <QUrl>

#include <AddressManager.h>

#include "ScreenshareScriptingInterface.h"

ScreenshareScriptingInterface::ScreenshareScriptingInterface() {
};

ScreenshareScriptingInterface::~ScreenshareScriptingInterface() {
    stopScreenshare();
}

void ScreenshareScriptingInterface::startScreenshare(const QString& roomName) {
    if (QThread::currentThread() != thread()) {
        // We must start a new QProcess from the main thread.
        QMetaObject::invokeMethod(
            this, "startScreenshare",
            Q_ARG(const QString&, roomName)
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

    QUuid currentDomainID = DependencyManager::get<AddressManager>()->getDomainID();

    // Make HTTP GET request to:
    // `https://metaverse.highfidelity.com/api/v1/domain/:domain_id/screenshare`,
    // passing the Domain ID that the user is connected to, as well as the `roomName`.
    // The server will respond with the relevant OpenTok Token, Session ID, and API Key.
    // Upon error-free response, do the logic below, passing in that info as necessary.
    QString token = "";
    QString apiKey = "";
    QString sessionID = "";

    QStringList arguments;
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
