//
//  ScreenshareScriptingInterface.cpp
//  interface/src/scripting/
//
//  Created by Milad Nazeri and Zach Fox on 2019-10-23.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QCoreApplication>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QUrl>
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcessEnvironment>

#include <AccountManager.h>
#include <AddressManager.h>
#include <EntityTreeRenderer.h>
#include <EntityTree.h>
#include <UUID.h>

#include "EntityScriptingInterface.h"
#include "ScreenshareScriptingInterface.h"

#include <RenderableEntityItem.h>
#include <RenderableTextEntityItem.h>
#include <RenderableWebEntityItem.h>

ScreenshareScriptingInterface::ScreenshareScriptingInterface() {
    auto esi = DependencyManager::get<EntityScriptingInterface>();
    if (!esi) {
        return;
    }

    QObject::connect(esi.data(), &EntityScriptingInterface::webEventReceived, this, &ScreenshareScriptingInterface::onWebEventReceived);
};

ScreenshareScriptingInterface::~ScreenshareScriptingInterface() {
    stopScreenshare();
}

static const EntityTypes::EntityType LOCAL_SCREENSHARE_WEB_ENTITY_TYPE = EntityTypes::Web;
static const uint8_t LOCAL_SCREENSHARE_WEB_ENTITY_FPS = 30;
// This is going to be a good amount of work to make this work dynamically for any screensize.
// V1 will have only hardcoded values.
static const glm::vec3 LOCAL_SCREENSHARE_WEB_ENTITY_LOCAL_POSITION(0.0f, -0.0862f, 0.0711f);
static const glm::vec3 LOCAL_SCREENSHARE_WEB_ENTITY_DIMENSIONS(4.0419f, 2.2735f, 0.0100f);
static const QString LOCAL_SCREENSHARE_WEB_ENTITY_URL =
    "https://hifi-content.s3.amazonaws.com/Experiences/Releases/usefulUtilities/smartBoard/screenshareViewer/screenshareClient.html";
void ScreenshareScriptingInterface::startScreenshare(const QUuid& screenshareZoneID,
                                                     const QUuid& smartboardEntityID,
                                                     const bool& isPresenter) {
    if (QThread::currentThread() != thread()) {
        // We must start a new QProcess from the main thread.
        QMetaObject::invokeMethod(this, "startScreenshare", Q_ARG(const QUuid&, screenshareZoneID),
                                  Q_ARG(const QUuid&, smartboardEntityID), Q_ARG(const bool&, isPresenter));
        return;
    }

    _screenshareZoneID = screenshareZoneID;
    _smartboardEntityID = smartboardEntityID;
    _isPresenter = isPresenter;

    if (_isPresenter && _screenshareProcess && _screenshareProcess->state() != QProcess::NotRunning) {
        return;
    }

    if (_isPresenter) {
        _screenshareProcess.reset(new QProcess(this));

        QFileInfo screenshareExecutable(SCREENSHARE_EXE_PATH);
        if (!screenshareExecutable.exists() || !screenshareExecutable.isFile()) {
            qDebug() << "Screenshare executable doesn't exist at" << SCREENSHARE_EXE_PATH;
            return;
        }
    }

    auto accountManager = DependencyManager::get<AccountManager>();
    if (!accountManager) {
        return;
    }
    auto addressManager = DependencyManager::get<AddressManager>();
    if (!addressManager) {
        return;
    }

    QString currentDomainID = uuidStringWithoutCurlyBraces(addressManager->getDomainID());
    QString requestURLPath = "api/v1/domains/%1/screenshare";

    JSONCallbackParameters callbackParams;
    callbackParams.callbackReceiver = this;
    callbackParams.jsonCallbackMethod = "handleSuccessfulScreenshareInfoGet";
    callbackParams.errorCallbackMethod = "handleFailedScreenshareInfoGet";
    accountManager->sendRequest(
        requestURLPath.arg(currentDomainID),
        AccountManagerAuth::Required,
        QNetworkAccessManager::GetOperation,
        callbackParams
    );
}

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
    _screenshareViewerLocalWebEntityUUID = "{00000000-0000-0000-0000-000000000000}";
    _token = "";
    _projectAPIKey = "";
    _sessionID = "";
    _isPresenter = false;
}

void ScreenshareScriptingInterface::handleSuccessfulScreenshareInfoGet(QNetworkReply* reply) {
    QString answer = reply->readAll();
    qDebug() << "\n\n MN HERE: REPLY" << answer;

    QByteArray answerByteArray = answer.toUtf8();
    QJsonDocument answerJSONObject = QJsonDocument::fromJson(answerByteArray);

    QString status = answerJSONObject["status"].toString();
    if (status == "fail") {
        qDebug() << "\n\n MN HERE: SCREENSHARE REPLY FAIL";
        return;
    }

    _token = answerJSONObject["token"].toString();
    _projectAPIKey = answerJSONObject["projectAPIKey"].toString();
    _sessionID = answerJSONObject["sessionID"].toString();
    qDebug() << "token:" << _token << " projectAPIKey:" << _projectAPIKey << " sessionID: " << _sessionID;

    if (_isPresenter) {
        QStringList arguments;
        arguments << " ";
        arguments << "--token=" + _token << " ";
        arguments << "--projectAPIKey=" + _projectAPIKey << " ";
        arguments << "--sessionID=" + _sessionID << " ";

        connect(_screenshareProcess.get(), &QProcess::errorOccurred,
            [=](QProcess::ProcessError error) { qDebug() << "ZRF QProcess::errorOccurred. `error`:" << error; });
        connect(_screenshareProcess.get(), &QProcess::started, [=]() { qDebug() << "ZRF QProcess::started"; });
        connect(_screenshareProcess.get(), &QProcess::stateChanged, [=](QProcess::ProcessState newState) {
            qDebug() << "ZRF QProcess::stateChanged. `newState`:" << newState;
            });
        connect(_screenshareProcess.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                qDebug() << "ZRF QProcess::finished. `exitCode`:" << exitCode << "`exitStatus`:" << exitStatus;
                emit screenshareStopped();
                stopScreenshare();
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
    localScreenshareWebEntityProps.setParentID(_smartboardEntityID);
    localScreenshareWebEntityProps.setDimensions(LOCAL_SCREENSHARE_WEB_ENTITY_DIMENSIONS);

    // The lines below will be used when writing the feature to ensure that the smartboard can be of any arbitrary size.
    //EntityPropertyFlags desiredSmartboardProperties;
    //desiredSmartboardProperties += PROP_POSITION;
    //desiredSmartboardProperties += PROP_DIMENSIONS;
    //EntityItemProperties smartboardProps = esi->getEntityProperties(smartboardEntityID, desiredSmartboardProperties);

    QString hostType = "local";
    _screenshareViewerLocalWebEntityUUID = esi->addEntity(localScreenshareWebEntityProps, hostType);
}

void ScreenshareScriptingInterface::handleFailedScreenshareInfoGet(QNetworkReply* reply) {
    qDebug() << "\n\n MN HERE: handleFailedScreenshareInfoGet():" << reply->readAll();
}

void ScreenshareScriptingInterface::onWebEventReceived(const QUuid& entityID, const QVariant& message) {
    if (entityID == _screenshareViewerLocalWebEntityUUID) {
        auto esi = DependencyManager::get<EntityScriptingInterface>();
        if (!esi) {
            return;
        }

        QByteArray jsonByteArray = QVariant(message).toString().toUtf8();
        QJsonDocument jsonObject = QJsonDocument::fromJson(jsonByteArray);

        if (jsonObject["app"] != "screenshare") {
            return;
        }

        if (jsonObject["method"] == "eventBridgeReady") {
            QJsonObject responseObject;
            responseObject.insert("app", "screenshare");
            responseObject.insert("method", "receiveConnectionInfo");
            QJsonObject responseObjectData;
            responseObjectData.insert("token", _token);
            responseObjectData.insert("projectAPIKey", _projectAPIKey);
            responseObjectData.insert("sessionID", _sessionID);
            responseObject.insert("data", responseObjectData);

            auto esi = DependencyManager::get<EntityScriptingInterface>();
            esi->emitScriptEvent(_screenshareViewerLocalWebEntityUUID, responseObject.toVariantMap());
        }
    }
}
