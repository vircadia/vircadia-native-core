//
//  AvatarHashMap.cpp
//  libraries/avatars/src
//
//  Created by Andrew Meadows on 1/28/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDataStream>

#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include <SharedUtil.h>

#include "AvatarLogging.h"
#include "AvatarHashMap.h"

AvatarHashMap::AvatarHashMap() {
    connect(DependencyManager::get<NodeList>().data(), &NodeList::uuidChanged, this, &AvatarHashMap::sessionUUIDChanged);
}

bool AvatarHashMap::isAvatarInRange(const glm::vec3& position, const float range) {
    foreach(const AvatarSharedPointer& sharedAvatar, _avatarHash) {
        glm::vec3 avatarPosition = sharedAvatar->getPosition();
        float distance = glm::distance(avatarPosition, position);
        if (distance < range) {
            return true;
        }
    }
    return false;
}

AvatarSharedPointer AvatarHashMap::newSharedAvatar() {
    return std::make_shared<AvatarData>();
}

AvatarSharedPointer AvatarHashMap::addAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer) {
    qCDebug(avatars) << "Adding avatar with sessionUUID " << sessionUUID << "to AvatarHashMap.";

    AvatarSharedPointer avatar = newSharedAvatar();
    avatar->setSessionUUID(sessionUUID);
    avatar->setOwningAvatarMixer(mixerWeakPointer);
    _avatarHash.insert(sessionUUID, avatar);

    return avatar;
}

void AvatarHashMap::processAvatarDataPacket(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode) {

    // enumerate over all of the avatars in this packet
    // only add them if mixerWeakPointer points to something (meaning that mixer is still around)
    while (packet->bytesLeftToRead()) {
        QUuid sessionUUID = QUuid::fromRfc4122(packet->read(NUM_BYTES_RFC4122_UUID));
        
        int positionBeforeRead = packet->pos();

        QByteArray byteArray = packet->read(packet->bytesLeftToRead());
        
        if (sessionUUID != _lastOwnerSessionUUID) {
            AvatarSharedPointer avatar = _avatarHash.value(sessionUUID);
            if (!avatar) {
                avatar = addAvatar(sessionUUID, sendingNode);
            }
            
            // have the matching (or new) avatar parse the data from the packet
            int bytesRead = avatar->parseDataFromBuffer(byteArray);
            packet->seek(positionBeforeRead + bytesRead);
        } else {
            // create a dummy AvatarData class to throw this data on the ground
            AvatarData dummyData;
            int bytesRead = dummyData.parseDataFromBuffer(byteArray);
            packet->seek(positionBeforeRead + bytesRead);
        }
    }
}

void AvatarHashMap::processAvatarIdentityPacket(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode) {
    // setup a data stream to parse the packet
    QDataStream identityStream(packet.data());

    QUuid sessionUUID;
    
    while (!identityStream.atEnd()) {

        QUrl faceMeshURL, skeletonURL;
        QVector<AttachmentData> attachmentData;
        QString displayName;
        identityStream >> sessionUUID >> faceMeshURL >> skeletonURL >> attachmentData >> displayName;

        // mesh URL for a UUID, find avatar in our list
        AvatarSharedPointer avatar = _avatarHash.value(sessionUUID);
        if (!avatar) {
            avatar = addAvatar(sessionUUID, sendingNode);
        }
        if (avatar->getFaceModelURL() != faceMeshURL) {
            avatar->setFaceModelURL(faceMeshURL);
        }

        if (avatar->getSkeletonModelURL() != skeletonURL) {
            avatar->setSkeletonModelURL(skeletonURL);
        }

        if (avatar->getAttachmentData() != attachmentData) {
            avatar->setAttachmentData(attachmentData);
        }

        if (avatar->getDisplayName() != displayName) {
            avatar->setDisplayName(displayName);
        }
    }
}

void AvatarHashMap::processAvatarBillboardPacket(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode) {
    QUuid sessionUUID = QUuid::fromRfc4122(packet->read(NUM_BYTES_RFC4122_UUID));

    AvatarSharedPointer avatar = _avatarHash.value(sessionUUID);
    if (!avatar) {
        avatar = addAvatar(sessionUUID, sendingNode);
    }

    QByteArray billboard = packet->read(packet->bytesLeftToRead());
    if (avatar->getBillboard() != billboard) {
        avatar->setBillboard(billboard);
    }
}

void AvatarHashMap::processKillAvatar(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode) {
    // read the node id
    QUuid sessionUUID = QUuid::fromRfc4122(packet->read(NUM_BYTES_RFC4122_UUID));
    removeAvatar(sessionUUID);
}

void AvatarHashMap::removeAvatar(const QUuid& sessionUUID) {
    _avatarHash.remove(sessionUUID);
}

void AvatarHashMap::sessionUUIDChanged(const QUuid& sessionUUID, const QUuid& oldUUID) {
    _lastOwnerSessionUUID = oldUUID;
}
