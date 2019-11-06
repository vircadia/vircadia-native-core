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
#include <QJsonDocument>
#include <QThread>
#include <QUrl>

#include <AddressManager.h>

#include "EntityScriptingInterface.h"
#include "ScreenshareScriptingInterface.h"

ScreenshareScriptingInterface::ScreenshareScriptingInterface() {
};

ScreenshareScriptingInterface::~ScreenshareScriptingInterface() {
    stopScreenshare();
}

static const EntityTypes::EntityType LOCAL_SCREENSHARE_WEB_ENTITY_TYPE = EntityTypes::Web;
static const uint8_t LOCAL_SCREENSHARE_WEB_ENTITY_FPS = 30;
static const glm::vec3 LOCAL_SCREENSHARE_WEB_ENTITY_LOCAL_POSITION(0.0f, 0.0f, 0.1f);
static const QString LOCAL_SCREENSHARE_WEB_ENTITY_URL = "https://s3.amazonaws.com/hifi-content/Experiences/Releases/usefulUtilities/smartBoard/screenshareViewer/screenshareClient.html";
void ScreenshareScriptingInterface::startScreenshare(const QUuid& screenshareZoneID, const QUuid& smartboardEntityID, const bool& isPresenter) {
    if (QThread::currentThread() != thread()) {
        // We must start a new QProcess from the main thread.
        QMetaObject::invokeMethod(
            this, "startScreenshare",
            Q_ARG(const bool&, isPresenter)
        );
        return;
    }

    if (isPresenter && _screenshareProcess && _screenshareProcess->state() != QProcess::NotRunning) {
        qDebug() << "Screenshare process already running. Aborting...";
        return;
    }

    if (isPresenter) {
        _screenshareProcess.reset(new QProcess(this));

        QFileInfo screenshareExecutable(SCREENSHARE_EXE_PATH);
        if (!screenshareExecutable.exists() || !screenshareExecutable.isFile()) {
            qDebug() << "Screenshare executable doesn't exist at" << SCREENSHARE_EXE_PATH;
            return;
        }
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

    if (isPresenter) {
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
    }

    if (!_screenshareViewerLocalWebEntityUUID.isNull()) {
        return;
    }

    auto esi = DependencyManager::get<EntityScriptingInterface>();
    if (!esi) {
        return;
    }

    EntityItemProperties localScreenshareWebEntityProps;
    localScreenshareWebEntityProps.setType(LOCAL_SCREENSHARE_WEB_ENTITY_TYPE);
    localScreenshareWebEntityProps.setMaxFPS(LOCAL_SCREENSHARE_WEB_ENTITY_FPS);
    localScreenshareWebEntityProps.setLocalPosition(LOCAL_SCREENSHARE_WEB_ENTITY_LOCAL_POSITION);
    localScreenshareWebEntityProps.setSourceUrl(LOCAL_SCREENSHARE_WEB_ENTITY_URL);
    localScreenshareWebEntityProps.setParentID(smartboardEntityID);

    EntityPropertyFlags desiredSmartboardProperties;
    desiredSmartboardProperties += PROP_POSITION;
    desiredSmartboardProperties += PROP_DIMENSIONS;
    EntityItemProperties smartboardProps = esi->getEntityProperties(smartboardEntityID, desiredSmartboardProperties);

    localScreenshareWebEntityProps.setPosition(smartboardProps.getPosition());
    localScreenshareWebEntityProps.setDimensions(smartboardProps.getDimensions());

    _screenshareViewerLocalWebEntityUUID = esi->addEntity(localScreenshareWebEntityProps, "local");

    QObject::connect(esi.data(), &EntityScriptingInterface::webEventReceived, this, [&](const QUuid& entityID, const QVariant& message) {
        if (entityID == _screenshareViewerLocalWebEntityUUID) {
            auto esi = DependencyManager::get<EntityScriptingInterface>();
            if (!esi) {
                return;
            }

            QJsonDocument jsonMessage = QJsonDocument::fromVariant(message);
            QJsonObject jsonObject = jsonMessage.object();

            if (jsonObject["type"] == "eventbridge_ready") {
                QJsonObject responseObject;
                responseObject.insert("type", "receiveConnectionInfo");
                QJsonObject responseObjectData;
                responseObjectData.insert("token", token);
                responseObjectData.insert("projectAPIKey", apiKey);
                responseObjectData.insert("sessionID", sessionID);
                responseObject.insert("data", responseObjectData);

                esi->emitScriptEvent(_screenshareViewerLocalWebEntityUUID, responseObject.toVariantMap());
            }
        }
    });
};

void ScreenshareScriptingInterface::stopScreenshare() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "stopScreenshare");
        return;
    }

    if (_screenshareProcess && _screenshareProcess->state() != QProcess::NotRunning) {
        _screenshareProcess->terminate();
    }

    if (!_screenshareViewerLocalWebEntityUUID.isNull()) {
        auto esi = DependencyManager::get<EntityScriptingInterface>();
        if (esi) {
            esi->deleteEntity(_screenshareViewerLocalWebEntityUUID);
        }
    }
}
