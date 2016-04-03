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

QVector<QUuid> AvatarHashMap::getAvatarIdentifiers() {
    QReadLocker locker(&_hashLock);
    return _avatarHash.keys().toVector();
}

AvatarData* AvatarHashMap::getAvatar(QUuid avatarID) {
    // Null/Default-constructed QUuids will return MyAvatar
    return getAvatarBySessionID(avatarID).get();
}

bool AvatarHashMap::isAvatarInRange(const glm::vec3& position, const float range) {
    auto hashCopy = getHashCopy();
    foreach(const AvatarSharedPointer& sharedAvatar, hashCopy) {
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
    
    auto avatar = newSharedAvatar();
    avatar->setSessionUUID(sessionUUID);
    avatar->setOwningAvatarMixer(mixerWeakPointer);
    
    _avatarHash.insert(sessionUUID, avatar);
    emit avatarAddedEvent(sessionUUID);
    
    return avatar;
}

AvatarSharedPointer AvatarHashMap::newOrExistingAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer) {
    QWriteLocker locker(&_hashLock);
    
    auto avatar = _avatarHash.value(sessionUUID);
    
    if (!avatar) {
        avatar = addAvatar(sessionUUID, mixerWeakPointer);
    }
    
    return avatar;
}

AvatarSharedPointer AvatarHashMap::findAvatar(const QUuid& sessionUUID) {
    QReadLocker locker(&_hashLock);
    if (_avatarHash.contains(sessionUUID)) {
        return _avatarHash.value(sessionUUID);
    }
    return nullptr;
}

void AvatarHashMap::processAvatarDataPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    // enumerate over all of the avatars in this packet
    // only add them if mixerWeakPointer points to something (meaning that mixer is still around)
    while (message->getBytesLeftToRead()) {
        QUuid sessionUUID = QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));
        
        int positionBeforeRead = message->getPosition();

        QByteArray byteArray = message->readWithoutCopy(message->getBytesLeftToRead());
        
        if (sessionUUID != _lastOwnerSessionUUID) {
            auto avatar = newOrExistingAvatar(sessionUUID, sendingNode);
            
            // have the matching (or new) avatar parse the data from the packet
            int bytesRead = avatar->parseDataFromBuffer(byteArray);
            message->seek(positionBeforeRead + bytesRead);
        } else {
            // create a dummy AvatarData class to throw this data on the ground
            AvatarData dummyData;
            int bytesRead = dummyData.parseDataFromBuffer(byteArray);
            message->seek(positionBeforeRead + bytesRead);
        }
    }
}

void AvatarHashMap::processAvatarIdentityPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    // setup a data stream to parse the packet
    QDataStream identityStream(message->getMessage());

    QUuid sessionUUID;
    
    while (!identityStream.atEnd()) {

        QUrl faceMeshURL, skeletonURL;
        QVector<AttachmentData> attachmentData;
        QString displayName;
        identityStream >> sessionUUID >> faceMeshURL >> skeletonURL >> attachmentData >> displayName;

        // mesh URL for a UUID, find avatar in our list
        auto avatar = newOrExistingAvatar(sessionUUID, sendingNode);
        
        if (avatar->getSkeletonModelURL().isEmpty() || (avatar->getSkeletonModelURL() != skeletonURL)) {
            avatar->setSkeletonModelURL(skeletonURL); // Will expand "" to default and so will not continuously fire
        }

        if (avatar->getAttachmentData() != attachmentData) {
            avatar->setAttachmentData(attachmentData);
        }

        if (avatar->getDisplayName() != displayName) {
            avatar->setDisplayName(displayName);
        }
    }
}

void AvatarHashMap::processAvatarBillboardPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    QUuid sessionUUID = QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));

    auto avatar = newOrExistingAvatar(sessionUUID, sendingNode);

    QByteArray billboard = message->read(message->getBytesLeftToRead());
    if (avatar->getBillboard() != billboard) {
        avatar->setBillboard(billboard);
    }
}

void AvatarHashMap::processKillAvatar(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    // read the node id
    QUuid sessionUUID = QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));
    removeAvatar(sessionUUID);
}

void AvatarHashMap::removeAvatar(const QUuid& sessionUUID) {
    QWriteLocker locker(&_hashLock);
    
    auto removedAvatar = _avatarHash.take(sessionUUID);
    
    if (removedAvatar) {
        handleRemovedAvatar(removedAvatar);
    }
}

void AvatarHashMap::handleRemovedAvatar(const AvatarSharedPointer& removedAvatar) {
    qDebug() << "Removed avatar with UUID" << uuidStringWithoutCurlyBraces(removedAvatar->getSessionUUID())
        << "from AvatarHashMap";
    emit avatarRemovedEvent(removedAvatar->getSessionUUID());
}

void AvatarHashMap::sessionUUIDChanged(const QUuid& sessionUUID, const QUuid& oldUUID) {
    _lastOwnerSessionUUID = oldUUID;
    emit avatarSessionChangedEvent(sessionUUID, oldUUID);
}
