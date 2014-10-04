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
            return 2;
        case PacketTypeSilentAudioFrame:
            return 4;
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

        case PacketTypeEntityAddOrEdit:
        case PacketTypeEntityData:
            return VERSION_ENTITIES_SUPPORT_DIMENSIONS;

        case PacketTypeEntityErase:
            return 2;
        case PacketTypeAudioStreamStats:
            return 1;
        case PacketTypeMetavoxelData:
            return 4;
        case PacketTypeVoxelData:
            return VERSION_VOXELS_HAS_FILE_BREAKS;
        default:
            return 0;
    }
}

#define PACKET_TYPE_NAME_LOOKUP(x) case x:  return QString(#x);

QString nameForPacketType(PacketType type) {
    switch (type) {
        PACKET_TYPE_NAME_LOOKUP(PacketTypeUnknown);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeStunResponse);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeDomainList);
        PACKET_TYPE_NAME_LOOKUP(PacketTypePing);
        PACKET_TYPE_NAME_LOOKUP(PacketTypePingReply);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeKillAvatar);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeAvatarData);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeInjectAudio);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeMixedAudio);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeMicrophoneAudioNoEcho);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeMicrophoneAudioWithEcho);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeBulkAvatarData);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeSilentAudioFrame);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeEnvironmentData);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeDomainListRequest);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeRequestAssignment);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeCreateAssignment);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeDomainOAuthRequest);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeMuteEnvironment);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeAudioStreamStats);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeDataServerConfirm);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeVoxelQuery);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeVoxelData);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeVoxelSet);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeVoxelSetDestructive);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeVoxelErase);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeOctreeStats);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeJurisdiction);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeJurisdictionRequest);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeParticleQuery);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeParticleData);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeParticleAddOrEdit);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeParticleErase);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeParticleAddResponse);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeMetavoxelData);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeAvatarIdentity);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeAvatarBillboard);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeDomainConnectRequest);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeDomainServerRequireDTLS);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeNodeJsonStats);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeEntityQuery);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeEntityData);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeEntityAddOrEdit);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeEntityErase);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeEntityAddResponse);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeOctreeDataNack);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeVoxelEditNack);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeParticleEditNack);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeEntityEditNack);
        PACKET_TYPE_NAME_LOOKUP(PacketTypeSignedTransactionPayment);
        default:
            return QString("Type: ") + QString::number((int)type);
    }
    return QString("unexpected");
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
