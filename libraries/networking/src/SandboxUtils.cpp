//
//  SandboxUtils.cpp
//  libraries/networking/src
//
//  Created by Brad Hefta-Gaub on 2016-10-15.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QProcess>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>

#include <NumericalConstants.h>
#include <SharedUtil.h>
#include <RunningMarker.h>

#include "SandboxUtils.h"
#include "NetworkAccessManager.h"


void SandboxUtils::ifLocalSandboxRunningElse(std::function<void()> localSandboxRunningDoThis,
                                               std::function<void()> localSandboxNotRunningDoThat) {

    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkRequest sandboxStatus(SANDBOX_STATUS_URL);
    sandboxStatus.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    sandboxStatus.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
    QNetworkReply* reply = networkAccessManager.get(sandboxStatus);

    connect(reply, &QNetworkReply::finished, this, [reply, localSandboxRunningDoThis, localSandboxNotRunningDoThat]() {
        if (reply->error() == QNetworkReply::NoError) {
            auto statusData = reply->readAll();
            auto statusJson = QJsonDocument::fromJson(statusData);
            if (!statusJson.isEmpty()) {
                auto statusObject = statusJson.object();
                auto serversValue = statusObject.value("servers");
                if (!serversValue.isUndefined() && serversValue.isObject()) {
                    auto serversObject = serversValue.toObject();
                    auto serversCount = serversObject.size();
                    const int MINIMUM_EXPECTED_SERVER_COUNT = 5;
                    if (serversCount >= MINIMUM_EXPECTED_SERVER_COUNT) {
                        localSandboxRunningDoThis();
                        return;
                    }
                }
            }
        }
        localSandboxNotRunningDoThat();
    });
}


void SandboxUtils::runLocalSandbox(QString contentPath, bool autoShutdown, QString runningMarkerName) {
    QString applicationDirPath = QFileInfo(QCoreApplication::applicationFilePath()).path();
    QString serverPath = applicationDirPath + "/server-console/server-console.exe";
    qDebug() << "Application dir path is: " << applicationDirPath;
    qDebug() << "Server path is: " << serverPath;
    qDebug() << "autoShutdown: " << autoShutdown;

    bool hasContentPath = !contentPath.isEmpty();
    bool passArgs = autoShutdown || hasContentPath;

    QStringList args;

    if (passArgs) {
        args << "--";
    }

    if (hasContentPath) {
        QString serverContentPath = applicationDirPath + "/" + contentPath;
        args << "--contentPath" << serverContentPath;
    }

    if (autoShutdown) {
        QString interfaceRunningStateFile = RunningMarker::getMarkerFilePath(runningMarkerName);
        args << "--shutdownWatcher" << interfaceRunningStateFile;
    }

    qDebug() << applicationDirPath;
    qDebug() << "Launching sandbox with:" << args;
    qDebug() << QProcess::startDetached(serverPath, args);

    // Sleep a short amount of time to give the server a chance to start
    usleep(2000000); /// do we really need this??
}
