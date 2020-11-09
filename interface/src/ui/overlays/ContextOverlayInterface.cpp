//
//  ContextOverlayInterface.cpp
//  interface/src/ui/overlays
//
//  Created by Zach Fox on 2017-07-14.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ContextOverlayInterface.h"
#include "Application.h"

#include <EntityTreeRenderer.h>
#include <NetworkingConstants.h>
#include <MetaverseAPI.h>
#include <NetworkAccessManager.h>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <commerce/Ledger.h>

#include <PointerManager.h>

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

static const float CONTEXT_OVERLAY_TABLET_OFFSET = 30.0f; // Degrees
static const float CONTEXT_OVERLAY_TABLET_ORIENTATION = 210.0f; // Degrees
static const float CONTEXT_OVERLAY_TABLET_DISTANCE = 0.65F; // Meters
ContextOverlayInterface::ContextOverlayInterface() {
    // "context_overlay" debug log category disabled by default.
    // Create your own "qtlogging.ini" file and set your "QT_LOGGING_CONF" environment variable
    // if you'd like to enable/disable certain categories.
    // More details: http://doc.qt.io/qt-5/qloggingcategory.html#configuring-categories
    QLoggingCategory::setFilterRules(QStringLiteral("hifi.context_overlay.debug=false"));

    _entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
    _hmdScriptingInterface = DependencyManager::get<HMDScriptingInterface>();
    _tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
    _selectionScriptingInterface = DependencyManager::get<SelectionScriptingInterface>();

    _entityPropertyFlags += PROP_POSITION;
    _entityPropertyFlags += PROP_ROTATION;
    _entityPropertyFlags += PROP_MARKETPLACE_ID;
    _entityPropertyFlags += PROP_DIMENSIONS;
    _entityPropertyFlags += PROP_REGISTRATION_POINT;
    _entityPropertyFlags += PROP_CERTIFICATE_ID;
    _entityPropertyFlags += PROP_ENTITY_HOST_TYPE;
    _entityPropertyFlags += PROP_OWNING_AVATAR_ID;

    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>().data();
    connect(entityScriptingInterface, &EntityScriptingInterface::mousePressOnEntity, this, &ContextOverlayInterface::clickDownOnEntity);
    connect(entityScriptingInterface, &EntityScriptingInterface::mouseReleaseOnEntity, this, &ContextOverlayInterface::mouseReleaseOnEntity);
    connect(entityScriptingInterface, &EntityScriptingInterface::hoverEnterEntity, this, &ContextOverlayInterface::contextOverlays_hoverEnterEntity);
    connect(entityScriptingInterface, &EntityScriptingInterface::hoverLeaveEntity, this, &ContextOverlayInterface::contextOverlays_hoverLeaveEntity);

    connect(&qApp->getOverlays(), &Overlays::hoverEnterOverlay, this, &ContextOverlayInterface::contextOverlays_hoverEnterOverlay);
    connect(&qApp->getOverlays(), &Overlays::hoverLeaveOverlay, this, &ContextOverlayInterface::contextOverlays_hoverLeaveOverlay);

    connect(_tabletScriptingInterface->getTablet("com.highfidelity.interface.tablet.system"), &TabletProxy::tabletShownChanged, this, [&]() {
        if (_contextOverlayJustClicked && _hmdScriptingInterface->isMounted()) {
            QUuid tabletFrameID = _hmdScriptingInterface->getCurrentTabletFrameID();
            auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
            glm::quat cameraOrientation = qApp->getCamera().getOrientation();

            EntityItemProperties properties;
            float sensorToWorldScale = myAvatar->getSensorToWorldScale();
            properties.setPosition(myAvatar->getEyePosition() + glm::quat(glm::radians(glm::vec3(0.0f, CONTEXT_OVERLAY_TABLET_OFFSET, 0.0f))) * ((CONTEXT_OVERLAY_TABLET_DISTANCE * sensorToWorldScale) * (cameraOrientation * Vectors::FRONT)));
            properties.setRotation(cameraOrientation * glm::quat(glm::radians(glm::vec3(0.0f, CONTEXT_OVERLAY_TABLET_ORIENTATION, 0.0f))));
            DependencyManager::get<EntityScriptingInterface>()->editEntity(tabletFrameID, properties);
            _contextOverlayJustClicked = false;
        }
    });
    connect(entityScriptingInterface, &EntityScriptingInterface::deletingEntity, this, &ContextOverlayInterface::deletingEntity);

    {
        _selectionScriptingInterface->enableListHighlight("contextOverlayHighlightList", QVariantMap());
    }

    auto nodeList = DependencyManager::get<NodeList>();
    auto& packetReceiver = nodeList->getPacketReceiver();
    packetReceiver.registerListener(PacketType::ChallengeOwnershipReply,
        PacketReceiver::makeSourcedListenerReference<ContextOverlayInterface>(this, &ContextOverlayInterface::handleChallengeOwnershipReplyPacket));
    _challengeOwnershipTimeoutTimer.setSingleShot(true);
}

static const glm::u8vec3 CONTEXT_OVERLAY_COLOR = { 255, 255, 255 };
static const float CONTEXT_OVERLAY_INSIDE_DISTANCE = 1.0f; // in meters
static const float CONTEXT_OVERLAY_SIZE = 0.09f; // in meters, same x and y dims
static const float CONTEXT_OVERLAY_OFFSET_DISTANCE = 0.1f;
static const float CONTEXT_OVERLAY_OFFSET_ANGLE = 5.0f;
static const float CONTEXT_OVERLAY_UNHOVERED_ALPHA = 0.85f;
static const float CONTEXT_OVERLAY_HOVERED_ALPHA = 1.0f;
static const float CONTEXT_OVERLAY_UNHOVERED_PULSEMIN = 0.6f;
static const float CONTEXT_OVERLAY_UNHOVERED_PULSEMAX = 1.0f;
static const float CONTEXT_OVERLAY_UNHOVERED_PULSEPERIOD = 1.0f;

void ContextOverlayInterface::setEnabled(bool enabled) {
    _enabled = enabled;
    if (!enabled) {
        // Destroy any potentially-active ContextOverlays when disabling the interface
        createOrDestroyContextOverlay(EntityItemID(), PointerEvent());
    }
}

void ContextOverlayInterface::clickDownOnEntity(const EntityItemID& id, const PointerEvent& event) {
    if (_enabled && event.getButton() == PointerEvent::SecondaryButton && contextOverlayFilterPassed(id)) {
        _mouseDownEntity = id;
        _mouseDownEntityTimestamp = usecTimestampNow();
    } else if ((event.shouldFocus() || event.getButton() == PointerEvent::PrimaryButton) && id == _contextOverlayID) {
        qCDebug(context_overlay) << "Clicked Context Overlay. Entity ID:" << _currentEntityWithContextOverlay << "ID:" << id;
        emit contextOverlayClicked(_currentEntityWithContextOverlay);
        _contextOverlayJustClicked = true;
    } else {
        if (!_currentEntityWithContextOverlay.isNull()) {
            disableEntityHighlight(_currentEntityWithContextOverlay);
            destroyContextOverlay(_currentEntityWithContextOverlay, event);
        }
    }
}

static const float CONTEXT_OVERLAY_CLICK_HOLD_TIME_MSEC = 400.0f;
void ContextOverlayInterface::mouseReleaseOnEntity(const EntityItemID& entityItemID, const PointerEvent& event) {
    if (!_mouseDownEntity.isNull() && ((usecTimestampNow() - _mouseDownEntityTimestamp) > (CONTEXT_OVERLAY_CLICK_HOLD_TIME_MSEC * USECS_PER_MSEC))) {
        _mouseDownEntity = EntityItemID();
    }
    if (_enabled && event.getButton() == PointerEvent::SecondaryButton && contextOverlayFilterPassed(entityItemID) && _mouseDownEntity == entityItemID) {
        createOrDestroyContextOverlay(entityItemID, event);
    }
}

bool ContextOverlayInterface::createOrDestroyContextOverlay(const EntityItemID& entityItemID, const PointerEvent& event) {
    if (_enabled && event.getButton() == PointerEvent::SecondaryButton) {
        if (contextOverlayFilterPassed(entityItemID)) {
            if (event.getID() == PointerManager::MOUSE_POINTER_ID || DependencyManager::get<PointerManager>()->isMouse(event.getID())) {
                enableEntityHighlight(entityItemID);
            }

            qCDebug(context_overlay) << "Creating Context Overlay on top of entity with ID: " << entityItemID;

            // Add all necessary variables to the stack
            EntityItemProperties entityProperties = _entityScriptingInterface->getEntityProperties(entityItemID, _entityPropertyFlags);
            glm::vec3 cameraPosition = qApp->getCamera().getPosition();
            glm::vec3 entityDimensions = entityProperties.getDimensions();
            glm::vec3 entityPosition = entityProperties.getPosition();
            glm::vec3 registrationPoint = entityProperties.getRegistrationPoint();
            glm::vec3 contextOverlayPosition = entityProperties.getPosition();
            glm::vec2 contextOverlayDimensions;

            // Update the position of the overlay if the registration point of the entity
            // isn't default
            if (registrationPoint != glm::vec3(0.5f)) {
                glm::vec3 adjustPos = registrationPoint - glm::vec3(0.5f);
                entityPosition = entityPosition - (entityProperties.getRotation() * (adjustPos * entityDimensions));
            }

            enableEntityHighlight(entityItemID);

            AABox boundingBox = AABox(entityPosition - (entityDimensions / 2.0f), entityDimensions * 2.0f);

            // Update the cached Entity Marketplace ID
            _entityMarketplaceID = entityProperties.getMarketplaceID();


            if (!_currentEntityWithContextOverlay.isNull() && _currentEntityWithContextOverlay != entityItemID) {
                disableEntityHighlight(_currentEntityWithContextOverlay);
            }

            // Update the cached "Current Entity with Context Overlay" variable
            setCurrentEntityWithContextOverlay(entityItemID);

            // Here, we determine the position and dimensions of the Context Overlay.
            if (boundingBox.contains(cameraPosition)) {
                // If the camera is inside the box...
                // ...position the Context Overlay 1 meter in front of the camera.
                contextOverlayPosition = cameraPosition + CONTEXT_OVERLAY_INSIDE_DISTANCE * (qApp->getCamera().getOrientation() * Vectors::FRONT);
                contextOverlayDimensions = glm::vec2(CONTEXT_OVERLAY_SIZE, CONTEXT_OVERLAY_SIZE) * glm::distance(contextOverlayPosition, cameraPosition);
            } else {
                // Rotate the Context Overlay some number of degrees offset from the entity
                // along the line cast from your head to the entity's bounding box.
                glm::vec3 direction = glm::normalize(entityPosition - cameraPosition);
                float distance;
                BoxFace face;
                glm::vec3 normal;
                boundingBox.findRayIntersection(cameraPosition, direction, 1.0f / direction, distance, face, normal);
                float offsetAngle = -CONTEXT_OVERLAY_OFFSET_ANGLE;
                if (event.getID() == 1) { // "1" is left hand
                    offsetAngle *= -1.0f;
                }
                contextOverlayPosition = cameraPosition +
                    (glm::quat(glm::radians(glm::vec3(0.0f, offsetAngle, 0.0f)))) * (direction * (distance - CONTEXT_OVERLAY_OFFSET_DISTANCE));
                contextOverlayDimensions = glm::vec2(CONTEXT_OVERLAY_SIZE, CONTEXT_OVERLAY_SIZE) * glm::distance(contextOverlayPosition, cameraPosition);
            }

            // Finally, setup and draw the Context Overlay
            auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
            if (_contextOverlayID == UNKNOWN_ENTITY_ID || !entityScriptingInterface->isAddedEntity(_contextOverlayID)) {
                EntityItemProperties properties;
                properties.setType(EntityTypes::Image);
                properties.setAlpha(CONTEXT_OVERLAY_UNHOVERED_ALPHA);
                properties.getPulse().setMin(CONTEXT_OVERLAY_UNHOVERED_PULSEMIN);
                properties.getPulse().setMax(CONTEXT_OVERLAY_UNHOVERED_PULSEMAX);
                properties.getPulse().setColorMode(PulseMode::IN_PHASE);
                properties.setIgnorePickIntersection(false);
                properties.setRenderLayer(RenderLayer::FRONT);
                properties.setImageURL(PathUtils::resourcesUrl() + "images/inspect-icon.png");
                properties.setBillboardMode(BillboardMode::FULL);

                _contextOverlayID = entityScriptingInterface->addEntityInternal(properties, entity::HostType::LOCAL);
            }

            EntityItemProperties properties;
            properties.setPosition(contextOverlayPosition);
            properties.setDimensions(glm::vec3(contextOverlayDimensions, 0.01f));
            properties.setRotation(entityProperties.getRotation());
            properties.setVisible(true);
            entityScriptingInterface->editEntity(_contextOverlayID, properties);

            return true;
        }
    } else {
        if (!_currentEntityWithContextOverlay.isNull()) {
            disableEntityHighlight(_currentEntityWithContextOverlay);
            return destroyContextOverlay(_currentEntityWithContextOverlay, event);
        }
        return false;
    }
    return false;
}

bool ContextOverlayInterface::contextOverlayFilterPassed(const EntityItemID& entityItemID) {
    EntityItemProperties entityProperties = _entityScriptingInterface->getEntityProperties(entityItemID, _entityPropertyFlags);
    return (entityProperties.getCertificateID().length() != 0);
}

bool ContextOverlayInterface::destroyContextOverlay(const EntityItemID& entityItemID, const PointerEvent& event) {
    if (_contextOverlayID != UNKNOWN_ENTITY_ID) {
        qCDebug(context_overlay) << "Destroying Context Overlay on top of entity with ID: " << entityItemID;
        disableEntityHighlight(entityItemID);
        setCurrentEntityWithContextOverlay(QUuid());
        _entityMarketplaceID.clear();
        DependencyManager::get<EntityScriptingInterface>()->deleteEntity(_contextOverlayID);
        _contextOverlayID = UNKNOWN_ENTITY_ID;
        return true;
    }
    return false;
}

bool ContextOverlayInterface::destroyContextOverlay(const EntityItemID& entityItemID) {
    return ContextOverlayInterface::destroyContextOverlay(entityItemID, PointerEvent());
}

void ContextOverlayInterface::contextOverlays_hoverEnterOverlay(const QUuid& id, const PointerEvent& event) {
    if (_contextOverlayID == id) {
        qCDebug(context_overlay) << "Started hovering over Context Overlay. ID:" << id;
        EntityItemProperties properties;
        properties.setColor(CONTEXT_OVERLAY_COLOR);
        properties.getPulse().setColorMode(PulseMode::NONE);
        properties.getPulse().setPeriod(0.0f);
        properties.setAlpha(CONTEXT_OVERLAY_HOVERED_ALPHA);
        DependencyManager::get<EntityScriptingInterface>()->editEntity(_contextOverlayID, properties);
    }
}

void ContextOverlayInterface::contextOverlays_hoverLeaveOverlay(const QUuid& id, const PointerEvent& event) {
    if (_contextOverlayID == id) {
        qCDebug(context_overlay) << "Stopped hovering over Context Overlay. ID:" << id;
        EntityItemProperties properties;
        properties.setColor(CONTEXT_OVERLAY_COLOR);
        properties.getPulse().setColorMode(PulseMode::IN_PHASE);
        properties.getPulse().setPeriod(CONTEXT_OVERLAY_UNHOVERED_PULSEPERIOD);
        properties.setAlpha(CONTEXT_OVERLAY_UNHOVERED_ALPHA);
        DependencyManager::get<EntityScriptingInterface>()->editEntity(_contextOverlayID, properties);
    }
}

void ContextOverlayInterface::contextOverlays_hoverEnterEntity(const EntityItemID& id, const PointerEvent& event) {
    bool isMouse = event.getID() == PointerManager::MOUSE_POINTER_ID || DependencyManager::get<PointerManager>()->isMouse(event.getID());
    if (contextOverlayFilterPassed(id) && _enabled && !isMouse) {
        enableEntityHighlight(id);
    }
}

void ContextOverlayInterface::contextOverlays_hoverLeaveEntity(const EntityItemID& id, const PointerEvent& event) {
    bool isMouse = event.getID() == PointerManager::MOUSE_POINTER_ID || DependencyManager::get<PointerManager>()->isMouse(event.getID());
    if (_currentEntityWithContextOverlay != id && _enabled && !isMouse) {
        disableEntityHighlight(id);
    }
}

void ContextOverlayInterface::requestOwnershipVerification(const QUuid& entityID) {

    setLastInspectedEntity(entityID);

    EntityItemProperties entityProperties = _entityScriptingInterface->getEntityProperties(entityID, _entityPropertyFlags);

    auto nodeList = DependencyManager::get<NodeList>();

    if (entityProperties.verifyStaticCertificateProperties()) {
        if (entityProperties.getEntityHostType() == entity::HostType::AVATAR) {
                SharedNodePointer entityServer = nodeList->soloNodeOfType(NodeType::EntityServer);

                if (entityServer) {
                    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
                    QNetworkRequest networkRequest;
                    networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
                    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
                    QUrl requestURL = MetaverseAPI::getCurrentMetaverseServerURL();
                    requestURL.setPath("/api/v1/commerce/proof_of_purchase_status/transfer");
                    QJsonObject request;
                    request["certificate_id"] = entityProperties.getCertificateID();
                    networkRequest.setUrl(requestURL);

                    QNetworkReply* networkReply = NULL;
                    networkReply = networkAccessManager.put(networkRequest, QJsonDocument(request).toJson());

                    connect(networkReply, &QNetworkReply::finished, [=]() {
                        QJsonObject jsonObject = QJsonDocument::fromJson(networkReply->readAll()).object();
                        jsonObject = jsonObject["data"].toObject();

                        if (networkReply->error() == QNetworkReply::NoError) {
                            if (!jsonObject["invalid_reason"].toString().isEmpty()) {
                                qCDebug(entities) << "invalid_reason not empty";
                            } else if (jsonObject["transfer_status"].toArray().first().toString() == "failed") {
                                qCDebug(entities) << "'transfer_status' is 'failed'";
                            } else if (jsonObject["transfer_status"].toArray().first().toString() == "pending") {
                                qCDebug(entities) << "'transfer_status' is 'pending'";
                            } else {
                                QString ownerKey = jsonObject["transfer_recipient_key"].toString();

                                QByteArray id = entityID.toByteArray();
                                QByteArray text = DependencyManager::get<EntityTreeRenderer>()->getTree()->computeNonce(entityID, ownerKey);
                                QByteArray nodeToChallengeByteArray = entityProperties.getOwningAvatarID().toRfc4122();

                                int idByteArraySize = id.length();
                                int textByteArraySize = text.length();
                                int nodeToChallengeByteArraySize = nodeToChallengeByteArray.length();

                                auto challengeOwnershipPacket = NLPacket::create(PacketType::ChallengeOwnershipRequest,
                                    idByteArraySize + textByteArraySize + nodeToChallengeByteArraySize + 3 * sizeof(int),
                                    true);
                                challengeOwnershipPacket->writePrimitive(idByteArraySize);
                                challengeOwnershipPacket->writePrimitive(textByteArraySize);
                                challengeOwnershipPacket->writePrimitive(nodeToChallengeByteArraySize);
                                challengeOwnershipPacket->write(id);
                                challengeOwnershipPacket->write(text);
                                challengeOwnershipPacket->write(nodeToChallengeByteArray);
                                nodeList->sendPacket(std::move(challengeOwnershipPacket), *entityServer);

                                // Kickoff a 10-second timeout timer that marks the cert if we don't get an ownership response in time
                                if (thread() != QThread::currentThread()) {
                                    QMetaObject::invokeMethod(this, "startChallengeOwnershipTimer", Q_ARG(const EntityItemID&, entityID));
                                    return;
                                } else {
                                    startChallengeOwnershipTimer(entityID);
                                }
                            }
                        } else {
                            qCDebug(entities) << "Call failed with error" << networkReply->error() <<
                                "More info:" << networkReply->readAll();
                        }

                        networkReply->deleteLater();
                    });
                } else {
                    qCWarning(context_overlay) << "Couldn't get Entity Server!";
                }
        } else {
            // We don't currently verify ownership of entities that aren't Avatar Entities,
            // so they always pass Ownership Verification. It's necessary to emit this signal
            // so that the Inspection Certificate can continue its information-grabbing process.
            auto ledger = DependencyManager::get<Ledger>();
            emit ledger->updateCertificateStatus(entityID, (uint)(ledger->CERTIFICATE_STATUS_VERIFICATION_SUCCESS));
        }
    } else {
        auto ledger = DependencyManager::get<Ledger>();
        _challengeOwnershipTimeoutTimer.stop();
        emit ledger->updateCertificateStatus(entityID, (uint)(ledger->CERTIFICATE_STATUS_STATIC_VERIFICATION_FAILED));
        emit DependencyManager::get<WalletScriptingInterface>()->ownershipVerificationFailed(entityID);
        qCDebug(context_overlay) << "Entity" << entityID << "failed static certificate verification!";
    }
}

void ContextOverlayInterface::enableEntityHighlight(const EntityItemID& entityID) {
    _selectionScriptingInterface->addToSelectedItemsList("contextOverlayHighlightList", "entity", entityID);
}

void ContextOverlayInterface::disableEntityHighlight(const EntityItemID& entityID) {
    _selectionScriptingInterface->removeFromSelectedItemsList("contextOverlayHighlightList", "entity", entityID);
}

void ContextOverlayInterface::deletingEntity(const EntityItemID& entityID) {
    if (_currentEntityWithContextOverlay == entityID) {
        destroyContextOverlay(_currentEntityWithContextOverlay, PointerEvent());
    }
}

void ContextOverlayInterface::startChallengeOwnershipTimer(const EntityItemID& entityItemID) {
    auto ledger = DependencyManager::get<Ledger>();
    EntityItemProperties entityProperties = _entityScriptingInterface->getEntityProperties(entityItemID, _entityPropertyFlags);

    connect(&_challengeOwnershipTimeoutTimer, &QTimer::timeout, this, [=]() {
        qCDebug(entities) << "Ownership challenge timed out for" << entityItemID;
        emit ledger->updateCertificateStatus(entityItemID, (uint)(ledger->CERTIFICATE_STATUS_VERIFICATION_TIMEOUT));
        emit DependencyManager::get<WalletScriptingInterface>()->ownershipVerificationFailed(entityItemID);
    });

    _challengeOwnershipTimeoutTimer.start(5000);
}

void ContextOverlayInterface::handleChallengeOwnershipReplyPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode) {
    auto ledger = DependencyManager::get<Ledger>();

    _challengeOwnershipTimeoutTimer.stop();

    int idByteArraySize;
    int textByteArraySize;

    packet->readPrimitive(&idByteArraySize);
    packet->readPrimitive(&textByteArraySize);

    EntityItemID id(packet->read(idByteArraySize));
    QString text(packet->read(textByteArraySize));

    bool verificationSuccess = DependencyManager::get<EntityTreeRenderer>()->getTree()->verifyNonce(id, text);

    if (verificationSuccess) {
        emit ledger->updateCertificateStatus(id, (uint)(ledger->CERTIFICATE_STATUS_VERIFICATION_SUCCESS));
        emit DependencyManager::get<WalletScriptingInterface>()->ownershipVerificationSuccess(id);
    } else {
        emit ledger->updateCertificateStatus(id, (uint)(ledger->CERTIFICATE_STATUS_OWNER_VERIFICATION_FAILED));
        emit DependencyManager::get<WalletScriptingInterface>()->ownershipVerificationFailed(id);
    }
}
