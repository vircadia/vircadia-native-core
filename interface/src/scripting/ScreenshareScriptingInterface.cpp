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

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QThread>
#include <QUrl>

#include <AccountManager.h>
#include <AddressManager.h>
#include <DependencyManager.h>
#include <NetworkingConstants.h>
#include <NodeList.h>
#include <UUID.h>

#include "EntityScriptingInterface.h"
#include "ScreenshareScriptingInterface.h"
#include "ExternalResource.h"

static const int SCREENSHARE_INFO_REQUEST_RETRY_TIMEOUT_MS = 300;
ScreenshareScriptingInterface::ScreenshareScriptingInterface() {
    auto esi = DependencyManager::get<EntityScriptingInterface>();
    if (!esi) {
        return;
    }

    // This signal/slot connection is used when the screen share local web entity sends an event bridge message. 
    QObject::connect(esi.data(), &EntityScriptingInterface::webEventReceived, this, &ScreenshareScriptingInterface::onWebEventReceived);

    _requestScreenshareInfoRetryTimer = new QTimer;
    _requestScreenshareInfoRetryTimer->setSingleShot(true);
    _requestScreenshareInfoRetryTimer->setInterval(SCREENSHARE_INFO_REQUEST_RETRY_TIMEOUT_MS);
    connect(_requestScreenshareInfoRetryTimer, &QTimer::timeout, this, &ScreenshareScriptingInterface::requestScreenshareInfo);

    // This packet listener handles the packet containing information about the latest zone ID in which we are allowed to share.
    auto nodeList = DependencyManager::get<NodeList>();
    PacketReceiver& packetReceiver = nodeList->getPacketReceiver();
    packetReceiver.registerListener(PacketType::AvatarZonePresence,
        PacketReceiver::makeUnsourcedListenerReference<ScreenshareScriptingInterface>(this, &ScreenshareScriptingInterface::processAvatarZonePresencePacketOnClient));
};

ScreenshareScriptingInterface::~ScreenshareScriptingInterface() {
    stopScreenshare();
}

void ScreenshareScriptingInterface::processAvatarZonePresencePacketOnClient(QSharedPointer<ReceivedMessage> message) {
    QUuid zone = QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));

    if (zone.isNull()) {
        qWarning() << "Ignoring avatar zone presence packet that doesn't specify a zone.";
        return;
    }

    // Set the last known authorized screenshare zone ID to the zone that the Domain Server just told us about.
    _lastAuthorizedZoneID = zone;

    // If we had previously started the screenshare process but knew that we weren't going to be authorized to screenshare,
    // let's continue the screenshare process here.
    if (_waitingForAuthorization) {
        requestScreenshareInfo();
    }
}

static const int MAX_NUM_SCREENSHARE_INFO_REQUEST_RETRIES = 5;
void ScreenshareScriptingInterface::requestScreenshareInfo() {
    // If the screenshare zone that we're currently in (i.e. `startScreenshare()` was called) is different from
    // the zone in which we are authorized to screenshare...
    // ...return early here and wait for the DS to send us a packet containing this zone's ID.
    if (_screenshareZoneID != _lastAuthorizedZoneID) {
        qDebug() << "Client not yet authorized to screenshare. Waiting for authorization message from domain server...";
        _waitingForAuthorization = true;
        return;
    }

    _waitingForAuthorization = false;

    _requestScreenshareInfoRetries++;

    if (_requestScreenshareInfoRetries >= MAX_NUM_SCREENSHARE_INFO_REQUEST_RETRIES) {
        qDebug() << "Maximum number of retries for screenshare info exceeded. Screenshare will not function.";
        return;
    }

    // Don't continue with any more of this logic if we can't get the `AccountManager` or `AddressManager`.
    auto accountManager = DependencyManager::get<AccountManager>();
    if (!accountManager) {
        return;
    }
    auto addressManager = DependencyManager::get<AddressManager>();
    if (!addressManager) {
        return;
    }

    // Construct and send a request to the Metaverse to obtain the information
    // necessary to start the screen sharing process.
    // This request requires:
    //     1. The domain ID of the domain in which the user's avatar is present
    //     2. User authentication information that is automatically included when `sendRequest()` is passed
    //         with the `AccountManagerAuth::Required` argument.
    // Note that this request will only return successfully if the Domain Server has already registered
    // the user paired with the current domain with the Metaverse.
    // See `DomainServer::screensharePresence()` for more info about that.

    QString currentDomainID = uuidStringWithoutCurlyBraces(addressManager->getDomainID());
    QString requestURLPath = "/api/v1/domains/%1/screenshare";
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

static const EntityTypes::EntityType LOCAL_SCREENSHARE_WEB_ENTITY_TYPE = EntityTypes::Web;
static const uint8_t LOCAL_SCREENSHARE_WEB_ENTITY_FPS = 30;
// This is going to be a good amount of work to make this work dynamically for any screensize.
// V1 will have only hardcoded values.
// The `z` value here is dynamic.
static const glm::vec3 LOCAL_SCREENSHARE_WEB_ENTITY_LOCAL_POSITION(0.0128f, -0.0918f, 0.0f);
static const glm::vec3 LOCAL_SCREENSHARE_WEB_ENTITY_DIMENSIONS(3.6790f, 2.0990f, 0.0100f);
static const ExternalResource::Bucket LOCAL_SCREENSHARE_WEB_ENTITY_BUCKET = ExternalResource::Bucket::HF_Content;
static const QString LOCAL_SCREENSHARE_WEB_ENTITY_PATH = 
    "Experiences/Releases/usefulUtilities/smartBoard/screenshareViewer/screenshareClient.html";
static const QString LOCAL_SCREENSHARE_WEB_ENTITY_HOST_TYPE = "local";
void ScreenshareScriptingInterface::startScreenshare(const QUuid& screenshareZoneID,
                                                     const QUuid& smartboardEntityID,
                                                     const bool& isPresenter) {
    // We must start a new QProcess from the main thread.
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "startScreenshare", Q_ARG(const QUuid&, screenshareZoneID),
                                  Q_ARG(const QUuid&, smartboardEntityID), Q_ARG(const bool&, isPresenter));
        return;
    }

    // These three private member variables are set now so that they may be used later during asynchronous
    // callbacks.
    _screenshareZoneID = screenshareZoneID;
    _smartboardEntityID = smartboardEntityID;
    _isPresenter = isPresenter;

    // If we are presenting, and the screenshare process is already running, don't do anything else here.
    if (_isPresenter && _screenshareProcess && _screenshareProcess->state() != QProcess::NotRunning) {
        return;
    }

    // If we're presenting...
    if (_isPresenter) {
        // ...make sure we first reset this `std::unique_ptr`.
        _screenshareProcess.reset(new QProcess(this));

        // Ensure that the screenshare executable exists where we expect it to.
        // Error out and reset the screen share state machine if the executable doesn't exist.
        QFileInfo screenshareExecutable(SCREENSHARE_EXE_PATH);
        if (!screenshareExecutable.exists() || !(screenshareExecutable.isFile() || screenshareExecutable.isBundle())) {
            qDebug() << "Screenshare executable doesn't exist at" << SCREENSHARE_EXE_PATH;
            stopScreenshare();
            emit screenshareError();
            return;
        }
    }

    if (_requestScreenshareInfoRetryTimer && _requestScreenshareInfoRetryTimer->isActive()) {
        _requestScreenshareInfoRetryTimer->stop();
    }

    _requestScreenshareInfoRetries = 0;
    requestScreenshareInfo();
}

void ScreenshareScriptingInterface::stopScreenshare() {
    // We can only deal with our Screen Share `QProcess` on the main thread.
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "stopScreenshare");
        return;
    }

    // If the retry timer is active, stop it.
    if (_requestScreenshareInfoRetryTimer && _requestScreenshareInfoRetryTimer->isActive()) {
        _requestScreenshareInfoRetryTimer->stop();
    }

    // If the Screen Share process is running...
    if (_screenshareProcess && _screenshareProcess->state() != QProcess::NotRunning) {
        //...terminate it and make sure that scripts know we terminated it by emitting
        // `screenshareProcessTerminated()`.
        _screenshareProcess->terminate();
    }

    // Delete the local web entity if we know about it here.
    if (!_screenshareViewerLocalWebEntityUUID.isNull()) {
        auto esi = DependencyManager::get<EntityScriptingInterface>();
        if (esi) {
            esi->deleteEntity(_screenshareViewerLocalWebEntityUUID);
        }
    }

    // Reset all private member variables related to screen share here.
    _screenshareViewerLocalWebEntityUUID = "{00000000-0000-0000-0000-000000000000}";
    _token = "";
    _projectAPIKey = "";
    _sessionID = "";
    _isPresenter = false;
    _waitingForAuthorization = false;
}

// Called when the Metaverse returns the information necessary to start/view a screen share.
void ScreenshareScriptingInterface::handleSuccessfulScreenshareInfoGet(QNetworkReply* reply) {
    // Read the reply and get it into a format we understand.
    QString answer = reply->readAll();
    QByteArray answerByteArray = answer.toUtf8();
    QJsonDocument answerJSONObject = QJsonDocument::fromJson(answerByteArray);

    // This Metaverse endpoint will always return a status key/value pair of "success" if things went well.
    QString status = answerJSONObject["status"].toString();
    if (status != "success") {
        qDebug() << "Error when retrieving screenshare info via HTTP. Error:" << reply->errorString();
        stopScreenshare();
        emit screenshareError();
        return;
    }

    // Store the information necessary to start/view a screen share in these private member variables.
    _token = answerJSONObject["token"].toString();
    _projectAPIKey = answerJSONObject["projectApiKey"].toString();
    _sessionID = answerJSONObject["sessionID"].toString();

    // Make sure we have all of the info that we need.
    if (_token.isEmpty() || _projectAPIKey.isEmpty() || _sessionID.isEmpty()) {
        qDebug() << "Not all Screen Share information was retrieved from the backend. Stopping...";
        stopScreenshare();
        emit screenshareError();
        return;
    }

    // If we're presenting:
    // 1. Build a list of arguments that we're going to pass to the screen share Electron app.
    // 2. Make sure we connect a signal/slot to know when the user quits the Electron app.
    // 3. Start the screen share Electron app with the list of args from (1).
    if (_isPresenter) {
        QStringList arguments;
        arguments << " ";
        arguments << "--token=" + _token << " ";
        arguments << "--projectAPIKey=" + _projectAPIKey << " ";
        arguments << "--sessionID=" + _sessionID << " ";

        connect(_screenshareProcess.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                stopScreenshare();
                emit screenshareProcessTerminated();
            });

        _screenshareProcess->start(SCREENSHARE_EXE_PATH, arguments);
    }

    // Make sure we can grab the entity scripting interface. Error out if we can't.
    auto esi = DependencyManager::get<EntityScriptingInterface>();
    if (!esi) {
        stopScreenshare();
        emit screenshareError();
        return;
    }

    // If, for some reason, we already have a record of a screen share local Web entity, delete it.
    if (!_screenshareViewerLocalWebEntityUUID.isNull()) {
        esi->deleteEntity(_screenshareViewerLocalWebEntityUUID);
    }

    // Set up the entity properties associated with the screen share local Web entity.
    EntityItemProperties localScreenshareWebEntityProps;
    localScreenshareWebEntityProps.setType(LOCAL_SCREENSHARE_WEB_ENTITY_TYPE);
    localScreenshareWebEntityProps.setMaxFPS(LOCAL_SCREENSHARE_WEB_ENTITY_FPS);
    glm::vec3 localPosition(LOCAL_SCREENSHARE_WEB_ENTITY_LOCAL_POSITION);
    localPosition.z = _localWebEntityZOffset;
    localScreenshareWebEntityProps.setLocalPosition(localPosition);
    auto LOCAL_SCREENSHARE_WEB_ENTITY_URL = ExternalResource::getInstance()->getUrl(LOCAL_SCREENSHARE_WEB_ENTITY_BUCKET,
        LOCAL_SCREENSHARE_WEB_ENTITY_PATH);
    localScreenshareWebEntityProps.setSourceUrl(LOCAL_SCREENSHARE_WEB_ENTITY_URL);
    localScreenshareWebEntityProps.setParentID(_smartboardEntityID);
    localScreenshareWebEntityProps.setDimensions(LOCAL_SCREENSHARE_WEB_ENTITY_DIMENSIONS);

    // The lines below will be used when writing the feature to support scaling the Smartboard entity to any arbitrary size.
    //EntityPropertyFlags desiredSmartboardProperties;
    //desiredSmartboardProperties += PROP_POSITION;
    //desiredSmartboardProperties += PROP_DIMENSIONS;
    //EntityItemProperties smartboardProps = esi->getEntityProperties(_smartboardEntityID, desiredSmartboardProperties);
    
    // Add the screen share local Web entity to Interface's entity tree.
    // When the Web entity loads the page specified by `LOCAL_SCREENSHARE_WEB_ENTITY_URL`, it will broadcast an Event Bridge
    // message, which we will consume inside `ScreenshareScriptingInterface::onWebEventReceived()`.
    _screenshareViewerLocalWebEntityUUID = esi->addEntity(localScreenshareWebEntityProps, LOCAL_SCREENSHARE_WEB_ENTITY_HOST_TYPE);
}

void ScreenshareScriptingInterface::handleFailedScreenshareInfoGet(QNetworkReply* reply) {
    if (_requestScreenshareInfoRetries >= MAX_NUM_SCREENSHARE_INFO_REQUEST_RETRIES) {
        qDebug() << "Failed to get screenshare info via HTTP after" << MAX_NUM_SCREENSHARE_INFO_REQUEST_RETRIES << "retries. Error:" << reply->errorString();
        stopScreenshare();
        emit screenshareError();
        return;
    }

    _requestScreenshareInfoRetryTimer->start();
}

// This function will handle _all_ web events received via `EntityScriptingInterface::webEventReceived()`, including
// those not related to screen sharing.
void ScreenshareScriptingInterface::onWebEventReceived(const QUuid& entityID, const QVariant& message) {
    // Bail early if the entity that sent the Web event isn't the one we care about.
    if (entityID == _screenshareViewerLocalWebEntityUUID) {
        // Bail early if we can't grab the Entity Scripting Interface.
        auto esi = DependencyManager::get<EntityScriptingInterface>();
        if (!esi) {
            return;
        }

        // Web events received from the screen share Web JS will always be in stringified JSON format.
        QByteArray jsonByteArray = QVariant(message).toString().toUtf8();
        QJsonDocument jsonObject = QJsonDocument::fromJson(jsonByteArray);

        // It should never happen where the screen share Web JS sends a message without the `app` key's value
        // set to "screenshare".
        if (jsonObject["app"] != "screenshare") {
            return;
        }

        // The screen share Web JS only sends a message with one method: "eventBridgeReady". Handle it here.
        if (jsonObject["method"] == "eventBridgeReady") {
            // Stuff a JSON object full of information necessary for the screen share local Web entity
            // to connect to the screen share session associated with the room in which the user's avatar is standing.
            QJsonObject responseObject;
            responseObject.insert("app", "screenshare");
            responseObject.insert("method", "receiveConnectionInfo");
            QJsonObject responseObjectData;
            responseObjectData.insert("token", _token);
            responseObjectData.insert("projectAPIKey", _projectAPIKey);
            responseObjectData.insert("sessionID", _sessionID);
            responseObject.insert("data", responseObjectData);

            esi->emitScriptEvent(_screenshareViewerLocalWebEntityUUID, responseObject.toVariantMap());
        }
    }
}
