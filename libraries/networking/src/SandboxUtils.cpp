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

#include "SandboxUtils.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QProcess>

#include <NumericalConstants.h>
#include <SharedUtil.h>
#include <RunningMarker.h>

#include "NetworkAccessManager.h"
#include "NetworkLogging.h"

namespace SandboxUtils {

QNetworkReply* getStatus() {
    auto& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkRequest sandboxStatus(SANDBOX_STATUS_URL);
    sandboxStatus.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    sandboxStatus.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
    return networkAccessManager.get(sandboxStatus);
}

bool readStatus(QByteArray statusData) {
    auto statusJson = QJsonDocument::fromJson(statusData);

    if (!statusJson.isEmpty()) {
        auto statusObject = statusJson.object();
        auto serversValue = statusObject.value("servers");
        if (!serversValue.isUndefined() && serversValue.isObject()) {
            auto serversObject = serversValue.toObject();
            auto serversCount = serversObject.size();
            const int MINIMUM_EXPECTED_SERVER_COUNT = 5;
            if (serversCount >= MINIMUM_EXPECTED_SERVER_COUNT) {
                return true;
            }
        }
    }

    return false;
}

void runLocalSandbox(QString contentPath, bool autoShutdown, bool noUpdater) {
    QString serverPath = "./server-console/server-console.exe";
    qCDebug(networking) << "Server path is: " << serverPath;
    qCDebug(networking) << "autoShutdown: " << autoShutdown;
    qCDebug(networking) << "noUpdater: " << noUpdater;

    bool hasContentPath = !contentPath.isEmpty();
    bool passArgs = autoShutdown || hasContentPath || noUpdater;

    QStringList args;

    if (passArgs) {
        args << "--";
    }

    if (hasContentPath) {
        QString serverContentPath = "./" + contentPath;
        args << "--contentPath" << serverContentPath;
    }

    if (autoShutdown) {
        auto pid = QCoreApplication::applicationPid();
        args << "--shutdownWith" << QString::number(pid);
    }

    if (noUpdater) {
        args << "--noUpdater";
    }

    qCDebug(networking) << "Launching sandbox with:" << args;
    qCDebug(networking) << QProcess::startDetached(serverPath, args);
}

}
