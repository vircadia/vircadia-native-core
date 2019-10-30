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
#include <QProcess>
#include <QThread>
#include <QDesktopServices>
#include <QUrl>
#include <QCoreApplication>


ScreenshareScriptingInterface::ScreenshareScriptingInterface() {

};

void ScreenshareScriptingInterface::startScreenshare(QString displayName, QString userName, QString token, QString sessionID, QString apiKey, QString fileLocation) {
    if (QThread::currentThread() != thread()) {
        // We must start a new QProcess from the main thread.
        QMetaObject::invokeMethod(
            this, "startScreenshare", 
            Q_ARG(QString, displayName),
            Q_ARG(QString, userName),
            Q_ARG(QString, token),
            Q_ARG(QString, sessionID),
            Q_ARG(QString, apiKey), 
            Q_ARG(QString, fileLocation)
        );
        return;
    }

    qDebug() << "ZRF: Inside startScreenshare(). `SCREENSHARE_EXE_PATH`:" << SCREENSHARE_EXE_PATH;

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

    QProcess* electronProcess = new QProcess(this);

    connect(electronProcess, &QProcess::errorOccurred,
        [=](QProcess::ProcessError error) { qDebug() << "ZRF QProcess::errorOccurred. `error`:" << error; });
    connect(electronProcess, &QProcess::started, [=]() { qDebug() << "ZRF QProcess::started"; });
    connect(electronProcess, &QProcess::stateChanged,
        [=](QProcess::ProcessState newState) { qDebug() << "ZRF QProcess::stateChanged. `newState`:" << newState; });
    connect(electronProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [=](int exitCode, QProcess::ExitStatus exitStatus) {
            qDebug() << "ZRF QProcess::finished. `exitCode`:" << exitCode << "`exitStatus`:" << exitStatus;
        });

    // Note for Milad:
    // We'll have to have equivalent lines of code for MacOS.
#ifdef Q_OS_WIN
    electronProcess->start(fileLocation, arguments);
#endif
};
