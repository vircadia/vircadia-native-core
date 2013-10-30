//
//  PacketHeaders.cpp
//  hifi
//
//  Created by Stephen Birarda on 6/28/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <stdio.h>

#include <QtCore/QDebug>

#include "PacketHeaders.h"

PACKET_VERSION versionForPacketType(PACKET_TYPE type) {
    switch (type) {

        case PACKET_TYPE_MICROPHONE_AUDIO_NO_ECHO:
        case PACKET_TYPE_MICROPHONE_AUDIO_WITH_ECHO:
            return 2;

        case PACKET_TYPE_HEAD_DATA:
            return 11;
        
        case PACKET_TYPE_AVATAR_URLS:
            return 2;
            
        case PACKET_TYPE_AVATAR_FACE_VIDEO:
            return 2;

        case PACKET_TYPE_VOXEL_STATS:
            return 2;
       
        case PACKET_TYPE_DOMAIN:
        case PACKET_TYPE_DOMAIN_LIST_REQUEST:
        case PACKET_TYPE_DOMAIN_REPORT_FOR_DUTY:
            return 1;
        
        case PACKET_TYPE_VOXEL_QUERY:
            return 1;
        
        default:
            return 0;
    }
}

bool packetVersionMatch(unsigned char* packetHeader) {
    // currently this just checks if the version in the packet matches our return from versionForPacketType
    // may need to be expanded in the future for types and versions that take > than 1 byte
    if (packetHeader[1] == versionForPacketType(packetHeader[0]) || packetHeader[0] == PACKET_TYPE_STUN_RESPONSE) {
        return true;
    } else {
        qDebug("There is a packet version mismatch for packet with header %c\n", packetHeader[0]);
        return false;
    }
}

int populateTypeAndVersion(unsigned char* destinationHeader, PACKET_TYPE type) {
    destinationHeader[0] = type;
    destinationHeader[1] = versionForPacketType(type);
    
    // return the number of bytes written for pointer pushing
    return 2;
}

int numBytesForPacketType(const unsigned char* packetType) {
    if (packetType[0] == 255) {
        return 1 + numBytesForPacketType(packetType + 1);
    } else {
        return 1;
    }
}

int numBytesForPacketVersion(const unsigned char* packetVersion) {
    if (packetVersion[0] == 255) {
        return 1 + numBytesForPacketVersion(packetVersion + 1);
    } else {
        return 1;
    }
}

int numBytesForPacketHeader(const unsigned char* packetHeader) {
    // int numBytesType = numBytesForPacketType(packetHeader);
    // return numBytesType + numBytesForPacketVersion(packetHeader + numBytesType);
    
    // currently this need not be dynamic - there are 2 bytes for each packet header
    return 2;
}
