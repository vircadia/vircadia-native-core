//
//  PacketHeaders.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 6/28/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <math.h>

#include <QtCore/QDebug>

#include "NodeList.h"

#include "PacketHeaders.h"

int arithmeticCodingValueFromBuffer(const char* checkValue) {
    if (((uchar) *checkValue) < 255) {
        return *checkValue;
    } else {
        return 255 + arithmeticCodingValueFromBuffer(checkValue + 1);
    }
}

int numBytesArithmeticCodingFromBuffer(const char* checkValue) {
    if (((uchar) *checkValue) < 255) {
        return 1;
    } else {
        return 1 + numBytesArithmeticCodingFromBuffer(checkValue + 1);
    }
}

int packArithmeticallyCodedValue(int value, char* destination) {
    if (value < 255) {
        // less than 255, just pack our value
        destination[0] = (uchar) value;
        return 1;
    } else {
        // pack 255 and then recursively pack on
        ((unsigned char*)destination)[0] = 255;
        return 1 + packArithmeticallyCodedValue(value - 255, destination + 1);
    }
}

PacketVersion versionForPacketType(PacketType type) {
    switch (type) {
        case PacketTypeMicrophoneAudioNoEcho:
        case PacketTypeMicrophoneAudioWithEcho:
        case PacketTypeSilentAudioFrame:
            return 2;
        case PacketTypeMixedAudio:
            return 1;
        case PacketTypeAvatarData:
            return 3;
        case PacketTypeAvatarIdentity:
            return 1;
        case PacketTypeEnvironmentData:
            return 2;
        case PacketTypeDomainList:
        case PacketTypeDomainListRequest:
            return 3;
        case PacketTypeCreateAssignment:
        case PacketTypeRequestAssignment:
            return 2;
        case PacketTypeVoxelSet:
        case PacketTypeVoxelSetDestructive:
            return 1;
        case PacketTypeOctreeStats:
            return 1;
        case PacketTypeParticleData:
            return 1;
        case PacketTypeParticleErase:
            return 1;
        case PacketTypeEntityData:
            return VERSION_ENTITIES_SUPPORT_SPLIT_MTU;
        case PacketTypeEntityErase:
            return 2;
        case PacketTypeAudioStreamStats:
            return 1;
        default:
            return 0;
    }
}

QByteArray byteArrayWithPopulatedHeader(PacketType type, const QUuid& connectionUUID) {
    QByteArray freshByteArray(MAX_PACKET_HEADER_BYTES, 0);
    freshByteArray.resize(populatePacketHeader(freshByteArray, type, connectionUUID));
    return freshByteArray;
}

int populatePacketHeader(QByteArray& packet, PacketType type, const QUuid& connectionUUID) {
    if (packet.size() < numBytesForPacketHeaderGivenPacketType(type)) {
        packet.resize(numBytesForPacketHeaderGivenPacketType(type));
    }
    
    return populatePacketHeader(packet.data(), type, connectionUUID);
}

int populatePacketHeader(char* packet, PacketType type, const QUuid& connectionUUID) {
    int numTypeBytes = packArithmeticallyCodedValue(type, packet);
    packet[numTypeBytes] = versionForPacketType(type);
    
    char* position = packet + numTypeBytes + sizeof(PacketVersion);
    
    QUuid packUUID = connectionUUID.isNull() ? LimitedNodeList::getInstance()->getSessionUUID() : connectionUUID;
    
    QByteArray rfcUUID = packUUID.toRfc4122();
    memcpy(position, rfcUUID.constData(), NUM_BYTES_RFC4122_UUID);
    position += NUM_BYTES_RFC4122_UUID;
    
    if (!NON_VERIFIED_PACKETS.contains(type)) {
        // pack 16 bytes of zeros where the md5 hash will be placed once data is packed
        memset(position, 0, NUM_BYTES_MD5_HASH);
        position += NUM_BYTES_MD5_HASH;
    }
    
    // return the number of bytes written for pointer pushing
    return position - packet;
}

int numBytesForPacketHeader(const QByteArray& packet) {
    // returns the number of bytes used for the type, version, and UUID
    return numBytesArithmeticCodingFromBuffer(packet.data())
    + numHashBytesInPacketHeaderGivenPacketType(packetTypeForPacket(packet))
    + NUM_STATIC_HEADER_BYTES;
}

int numBytesForPacketHeader(const char* packet) {
    // returns the number of bytes used for the type, version, and UUID
    return numBytesArithmeticCodingFromBuffer(packet)
    + numHashBytesInPacketHeaderGivenPacketType(packetTypeForPacket(packet))
    + NUM_STATIC_HEADER_BYTES;
}

int numBytesForPacketHeaderGivenPacketType(PacketType type) {
    return (int) ceilf((float)type / 255)
    + numHashBytesInPacketHeaderGivenPacketType(type)
    + NUM_STATIC_HEADER_BYTES;
}

int numHashBytesInPacketHeaderGivenPacketType(PacketType type) {
    return (NON_VERIFIED_PACKETS.contains(type) ? 0 : NUM_BYTES_MD5_HASH);
}

QUuid uuidFromPacketHeader(const QByteArray& packet) {
    return QUuid::fromRfc4122(packet.mid(numBytesArithmeticCodingFromBuffer(packet.data()) + sizeof(PacketVersion),
                                         NUM_BYTES_RFC4122_UUID));
}

QByteArray hashFromPacketHeader(const QByteArray& packet) {
    return packet.mid(numBytesForPacketHeader(packet) - NUM_BYTES_MD5_HASH, NUM_BYTES_MD5_HASH);
}

QByteArray hashForPacketAndConnectionUUID(const QByteArray& packet, const QUuid& connectionUUID) {
    return QCryptographicHash::hash(packet.mid(numBytesForPacketHeader(packet)) + connectionUUID.toRfc4122(),
                                    QCryptographicHash::Md5);
}

void replaceHashInPacketGivenConnectionUUID(QByteArray& packet, const QUuid& connectionUUID) {
    packet.replace(numBytesForPacketHeader(packet) - NUM_BYTES_MD5_HASH, NUM_BYTES_MD5_HASH,
                   hashForPacketAndConnectionUUID(packet, connectionUUID));
}

PacketType packetTypeForPacket(const QByteArray& packet) {
    return (PacketType) arithmeticCodingValueFromBuffer(packet.data());
}

PacketType packetTypeForPacket(const char* packet) {
    return (PacketType) arithmeticCodingValueFromBuffer(packet);
}
