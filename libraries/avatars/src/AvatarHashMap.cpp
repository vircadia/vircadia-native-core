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

#include <PacketHeaders.h>

#include "AvatarHashMap.h"

AvatarHashMap::AvatarHashMap() :
    _avatarHash()
{
}

void AvatarHashMap::insert(const QUuid& id, AvatarSharedPointer avatar) {
    _avatarHash.insert(id, avatar);
    avatar->setSessionUUID(id);
}

AvatarHash::iterator AvatarHashMap::erase(const AvatarHash::iterator& iterator) {
    return _avatarHash.erase(iterator);
}

bool AvatarHashMap::shouldKillAvatar(const AvatarSharedPointer& sharedAvatar) {
    return (sharedAvatar->getOwningAvatarMixer() == NULL);
}

AvatarSharedPointer AvatarHashMap::newSharedAvatar() {
    AvatarData* avatarData = new AvatarData();
    return AvatarSharedPointer(avatarData);
}

AvatarSharedPointer AvatarHashMap::matchingOrNewAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer) {
    AvatarSharedPointer matchingAvatar = _avatarHash.value(sessionUUID);
    
    if (!matchingAvatar) {
        // insert the new avatar into our hash
        matchingAvatar = newSharedAvatar();
    
        qDebug() << "Adding avatar with sessionUUID " << sessionUUID << "to AvatarManager hash.";
        _avatarHash.insert(sessionUUID, matchingAvatar);
        
        matchingAvatar->setOwningAvatarMixer(mixerWeakPointer);
    }
    
    return matchingAvatar;
}

void AvatarHashMap::processAvatarDataPacket(const QByteArray &datagram, const QWeakPointer<Node> &mixerWeakPointer) {
    int bytesRead = numBytesForPacketHeader(datagram);
    
    // enumerate over all of the avatars in this packet
    // only add them if mixerWeakPointer points to something (meaning that mixer is still around)
    while (bytesRead < datagram.size() && mixerWeakPointer.data()) {
        QUuid sessionUUID = QUuid::fromRfc4122(datagram.mid(bytesRead, NUM_BYTES_RFC4122_UUID));
        bytesRead += NUM_BYTES_RFC4122_UUID;
        
        AvatarSharedPointer matchingAvatarData = matchingOrNewAvatar(sessionUUID, mixerWeakPointer);
        
        // have the matching (or new) avatar parse the data from the packet
        bytesRead += matchingAvatarData->parseDataAtOffset(datagram, bytesRead);
    }
}

void AvatarHashMap::processAvatarIdentityPacket(const QByteArray &packet, const QWeakPointer<Node>& mixerWeakPointer) {
    // setup a data stream to parse the packet
    QDataStream identityStream(packet);
    identityStream.skipRawData(numBytesForPacketHeader(packet));
    
    QUuid sessionUUID;
    
    while (!identityStream.atEnd()) {
        
        QUrl faceMeshURL, skeletonURL;
        QString displayName;
        identityStream >> sessionUUID >> faceMeshURL >> skeletonURL >> displayName;
        
        // mesh URL for a UUID, find avatar in our list
        AvatarSharedPointer matchingAvatar = matchingOrNewAvatar(sessionUUID, mixerWeakPointer);
        if (matchingAvatar) {
            
            if (matchingAvatar->getFaceModelURL() != faceMeshURL) {
                matchingAvatar->setFaceModelURL(faceMeshURL);
            }
            
            if (matchingAvatar->getSkeletonModelURL() != skeletonURL) {
                matchingAvatar->setSkeletonModelURL(skeletonURL);
            }
            
            if (matchingAvatar->getDisplayName() != displayName) {
                matchingAvatar->setDisplayName(displayName);
            }
        }
    }
}

void AvatarHashMap::processAvatarBillboardPacket(const QByteArray& packet, const QWeakPointer<Node>& mixerWeakPointer) {
    int headerSize = numBytesForPacketHeader(packet);
    QUuid sessionUUID = QUuid::fromRfc4122(QByteArray::fromRawData(packet.constData() + headerSize, NUM_BYTES_RFC4122_UUID));
    
    AvatarSharedPointer matchingAvatar = matchingOrNewAvatar(sessionUUID, mixerWeakPointer);
    if (matchingAvatar) {
        QByteArray billboard = packet.mid(headerSize + NUM_BYTES_RFC4122_UUID);
        if (matchingAvatar->getBillboard() != billboard) {
            matchingAvatar->setBillboard(billboard);
        }
    }
}

void AvatarHashMap::processKillAvatar(const QByteArray& datagram) {
    // read the node id
    QUuid sessionUUID = QUuid::fromRfc4122(datagram.mid(numBytesForPacketHeader(datagram), NUM_BYTES_RFC4122_UUID));
    
    // remove the avatar with that UUID from our hash, if it exists
    AvatarHash::iterator matchedAvatar = _avatarHash.find(sessionUUID);
    if (matchedAvatar != _avatarHash.end()) {
        erase(matchedAvatar);
    }
}