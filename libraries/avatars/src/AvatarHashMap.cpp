//
//  AvatarHashMap.cpp
//  libraries/avatars/src
//
//  Created by AndrewMeadows on 1/28/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDataStream>

#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>

#include "AvatarLogging.h"
#include "AvatarHashMap.h"

AvatarHashMap::AvatarHashMap() {
    connect(DependencyManager::get<NodeList>().data(), &NodeList::uuidChanged, this, &AvatarHashMap::sessionUUIDChanged);
}

void AvatarHashMap::processAvatarMixerDatagram(const QByteArray& datagram, const QWeakPointer<Node>& mixerWeakPointer) {
    switch (packetTypeForPacket(datagram)) {
        case PacketTypeBulkAvatarData:
            processAvatarDataPacket(datagram, mixerWeakPointer);
            break;
        case PacketTypeAvatarIdentity:
            processAvatarIdentityPacket(datagram, mixerWeakPointer);
            break;
        case PacketTypeAvatarBillboard:
            processAvatarBillboardPacket(datagram, mixerWeakPointer);
            break;
        case PacketTypeKillAvatar:
            processKillAvatar(datagram);
            break;
        default:
            break;
    }
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
    return AvatarSharedPointer(new AvatarData());
}

AvatarSharedPointer AvatarHashMap::addAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer) {
    qCDebug(avatars) << "Adding avatar with sessionUUID " << sessionUUID << "to AvatarHashMap.";

    AvatarSharedPointer avatar = newSharedAvatar();
    avatar->setSessionUUID(sessionUUID);
    avatar->setOwningAvatarMixer(mixerWeakPointer);
    _avatarHash.insert(sessionUUID, avatar);

    return avatar;
}

void AvatarHashMap::processAvatarDataPacket(const QByteArray &datagram, const QWeakPointer<Node> &mixerWeakPointer) {
    int bytesRead = numBytesForPacketHeader(datagram);
    
    // enumerate over all of the avatars in this packet
    // only add them if mixerWeakPointer points to something (meaning that mixer is still around)
    while (bytesRead < datagram.size() && mixerWeakPointer.data()) {
        QUuid sessionUUID = QUuid::fromRfc4122(datagram.mid(bytesRead, NUM_BYTES_RFC4122_UUID));
        bytesRead += NUM_BYTES_RFC4122_UUID;
        
        if (sessionUUID != _lastOwnerSessionUUID) {
            AvatarSharedPointer avatar = _avatarHash.value(sessionUUID);
            if (!avatar) {
                avatar = addAvatar(sessionUUID, mixerWeakPointer);
            }
            
            // have the matching (or new) avatar parse the data from the packet
            bytesRead += avatar->parseDataAtOffset(datagram, bytesRead);
        } else {
            // create a dummy AvatarData class to throw this data on the ground
            AvatarData dummyData;
            bytesRead += dummyData.parseDataAtOffset(datagram, bytesRead);
        }
    }
}

void AvatarHashMap::processAvatarIdentityPacket(const QByteArray &packet, const QWeakPointer<Node>& mixerWeakPointer) {
    // setup a data stream to parse the packet
    QDataStream identityStream(packet);
    identityStream.skipRawData(numBytesForPacketHeader(packet));
    
    QUuid sessionUUID;
    
    while (!identityStream.atEnd()) {
        
        QUrl faceMeshURL, skeletonURL;
        QVector<AttachmentData> attachmentData;
        QString displayName;
        identityStream >> sessionUUID >> faceMeshURL >> skeletonURL >> attachmentData >> displayName;
        
        // mesh URL for a UUID, find avatar in our list
        AvatarSharedPointer avatar = _avatarHash.value(sessionUUID);
        if (!avatar) {
            avatar = addAvatar(sessionUUID, mixerWeakPointer);
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

void AvatarHashMap::processAvatarBillboardPacket(const QByteArray& packet, const QWeakPointer<Node>& mixerWeakPointer) {
    int headerSize = numBytesForPacketHeader(packet);
    QUuid sessionUUID = QUuid::fromRfc4122(QByteArray::fromRawData(packet.constData() + headerSize, NUM_BYTES_RFC4122_UUID));
    
    AvatarSharedPointer avatar = _avatarHash.value(sessionUUID);
    if (!avatar) {
        avatar = addAvatar(sessionUUID, mixerWeakPointer);
    }

    QByteArray billboard = packet.mid(headerSize + NUM_BYTES_RFC4122_UUID);
    if (avatar->getBillboard() != billboard) {
        avatar->setBillboard(billboard);
    }
}

void AvatarHashMap::processKillAvatar(const QByteArray& datagram) {
    // read the node id
    QUuid sessionUUID = QUuid::fromRfc4122(datagram.mid(numBytesForPacketHeader(datagram), NUM_BYTES_RFC4122_UUID));
    removeAvatar(sessionUUID);
}

void AvatarHashMap::removeAvatar(const QUuid& sessionUUID) {
    _avatarHash.remove(sessionUUID);
}

void AvatarHashMap::sessionUUIDChanged(const QUuid& sessionUUID, const QUuid& oldUUID) {
    _lastOwnerSessionUUID = oldUUID;
}
