//
//  EntityEditPacketSender.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <assert.h>
#include <QJsonDocument>
#include <PerfStat.h>
#include <OctalCode.h>
#include <udt/PacketHeaders.h>
#include "EntityEditPacketSender.h"
#include "EntitiesLogging.h"
#include "EntityItem.h"
#include "EntityItemProperties.h"

EntityEditPacketSender::EntityEditPacketSender() {
    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerDirectListener(PacketType::EntityEditNack, this, "processEntityEditNackPacket");
}

void EntityEditPacketSender::processEntityEditNackPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    processNackPacket(*message, sendingNode);
}

void EntityEditPacketSender::adjustEditPacketForClockSkew(PacketType type, QByteArray& buffer, qint64 clockSkew) {
    if (type == PacketType::EntityAdd || type == PacketType::EntityEdit) {
        EntityItem::adjustEditPacketForClockSkew(buffer, clockSkew);
    }
}

void EntityEditPacketSender::queueEditAvatarEntityMessage(PacketType type,
                                                          EntityTreePointer entityTree,
                                                          EntityItemID entityItemID,
                                                          const EntityItemProperties& properties) {
    if (!_shouldSend) {
        return; // bail early
    }

    if (properties.getOwningAvatarID() != _myAvatar->getID()) {
        return; // don't send updates for someone else's avatarEntity
    }

    assert(properties.getClientOnly());

    // this is an avatar-based entity.  update our avatar-data rather than sending to the entity-server
    assert(_myAvatar);

    if (!entityTree) {
        qDebug() << "EntityEditPacketSender::queueEditEntityMessage null entityTree.";
        return;
    }
    EntityItemPointer entity = entityTree->findEntityByEntityItemID(entityItemID);
    if (!entity) {
        qDebug() << "EntityEditPacketSender::queueEditEntityMessage can't find entity.";
        return;
    }

    // the properties that get serialized into the avatar identity packet should be the entire set
    // rather than just the ones being edited.
    EntityItemProperties entityProperties = entity->getProperties();
    entityProperties.merge(properties);

    QScriptValue scriptProperties = EntityItemNonDefaultPropertiesToScriptValue(&_scriptEngine, entityProperties);
    QVariant variantProperties = scriptProperties.toVariant();
    QJsonDocument jsonProperties = QJsonDocument::fromVariant(variantProperties);

    // the ID of the parent/avatar changes from session to session.  use a special UUID to indicate the avatar
    QJsonObject jsonObject = jsonProperties.object();
    if (QUuid(jsonObject["parentID"].toString()) == _myAvatar->getID()) {
        jsonObject["parentID"] = AVATAR_SELF_ID.toString();
    }
    jsonProperties = QJsonDocument(jsonObject);

    QByteArray binaryProperties = jsonProperties.toBinaryData();
    _myAvatar->updateAvatarEntity(entityItemID, binaryProperties);

    entity->setLastBroadcast(usecTimestampNow());
    return;
}


void EntityEditPacketSender::queueEditEntityMessage(PacketType type,
                                                    EntityTreePointer entityTree,
                                                    EntityItemID entityItemID,
                                                    const EntityItemProperties& properties) {
    if (!_shouldSend) {
        return; // bail early
    }

    if (properties.getClientOnly()) {
        queueEditAvatarEntityMessage(type, entityTree, entityItemID, properties);
        return;
    }

    QByteArray bufferOut(NLPacket::maxPayloadSize(type), 0);

    if (EntityItemProperties::encodeEntityEditPacket(type, entityItemID, properties, bufferOut)) {
        #ifdef WANT_DEBUG
            qCDebug(entities) << "calling queueOctreeEditMessage()...";
            qCDebug(entities) << "    id:" << entityItemID;
            qCDebug(entities) << "    properties:" << properties;
        #endif
        queueOctreeEditMessage(type, bufferOut);
    }
}

void EntityEditPacketSender::queueEraseEntityMessage(const EntityItemID& entityItemID) {
    if (!_shouldSend) {
        return; // bail early
    }

    // in case this was a clientOnly entity:
    if(_myAvatar) {
        _myAvatar->clearAvatarEntity(entityItemID);
    }

    QByteArray bufferOut(NLPacket::maxPayloadSize(PacketType::EntityErase), 0);

    if (EntityItemProperties::encodeEraseEntityMessage(entityItemID, bufferOut)) {
        queueOctreeEditMessage(PacketType::EntityErase, bufferOut);
    }
}
