//
//  AvatarManager.cpp
//  hifi
//
//  Created by Stephen Birarda on 1/23/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <UUID.h>

#include "Avatar.h"

#include "AvatarManager.h"

AvatarManager::AvatarManager(QObject* parent) :
    _hash()
{
    
}

void AvatarManager::processAvatarMixerDatagram(const QByteArray& datagram) {
    unsigned char packetData[MAX_PACKET_SIZE];
    memcpy(packetData, datagram.data(), datagram.size());
    
    int numBytesPacketHeader = numBytesForPacketHeader(packetData);

    QUuid nodeUUID = QUuid::fromRfc4122(datagram.mid(numBytesPacketHeader, NUM_BYTES_RFC4122_UUID));
    
    int bytesRead = numBytesPacketHeader;
    
    unsigned char avatarData[MAX_PACKET_SIZE];
    populateTypeAndVersion(avatarData, PACKET_TYPE_HEAD_DATA);

    while (bytesRead < datagram.size()) {
        Avatar* matchingAvatar = _hash.value(nodeUUID);
        
        if (!matchingAvatar) {
            // construct a new Avatar for this node
            matchingAvatar = new Avatar();
            
            // insert the new avatar into our hash
            _hash.insert(nodeUUID, matchingAvatar);
            
            qDebug() << "Adding avatar with UUID" << nodeUUID << "to AvatarManager hash.";
        }
        
        // copy the rest of the packet to the avatarData holder so we can read the next Avatar from there
        memcpy(avatarData, packetData + bytesRead, datagram.size() - bytesRead);
        
        // have the matching (or new) avatar parse the data from the packet
        bytesRead += matchingAvatar->parseData(avatarData,
                                               datagram.size() - bytesRead);
    }
}

void AvatarManager::processKillAvatar(const QByteArray& datagram) {
    // read the node id
    QUuid nodeUUID = QUuid::fromRfc4122(datagram.mid(numBytesForPacketHeader(reinterpret_cast<const unsigned char*>
                                                                             (datagram.data())),
                                                     NUM_BYTES_RFC4122_UUID));
    
    // kill the avatar with that UUID from our hash, if it exists
    _hash.remove(nodeUUID);
}

void AvatarManager::clearHash() {
    // clear the AvatarManager hash - typically happens on the removal of the avatar-mixer
    _hash.clear();
}