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

#include "PacketHeaders.h"

#include <math.h>

#include <QtCore/QDebug>

using namespace PacketType;

const QSet<PacketType::Value> NON_VERIFIED_PACKETS = QSet<PacketType::Value>()
    << DomainServerRequireDTLS << DomainConnectRequest
    << DomainList << DomainListRequest << DomainConnectionDenied
    << CreateAssignment << RequestAssignment << StunResponse
    << NodeJsonStats << EntityQuery
    << OctreeDataNack << EntityEditNack
    << IceServerHeartbeat << IceServerPeerInformation
    << IceServerQuery << UnverifiedPing
    << UnverifiedPingReply << StopNode
    << DomainServerPathQuery << DomainServerPathResponse
    << DomainServerAddedNode;

const QSet<PacketType::Value> SEQUENCE_NUMBERED_PACKETS = QSet<PacketType::Value>() << AvatarData;

const QSet<PacketType::Value> NON_SOURCED_PACKETS = QSet<PacketType::Value>()
    << ICEPing << ICEPingReply << DomainConnectRequest;

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

PacketVersion versionForPacketType(PacketType::Value packetType) {
    switch (packetType) {
        case MicrophoneAudioNoEcho:
        case MicrophoneAudioWithEcho:
            return 2;
        case SilentAudioFrame:
            return 4;
        case MixedAudio:
            return 1;
        case InjectAudio:
            return 1;
        case AvatarData:
            return 6;
        case AvatarIdentity:
            return 1;
        case EnvironmentData:
            return 2;
        case DomainList:
        case DomainListRequest:
            return 5;
        case CreateAssignment:
        case RequestAssignment:
            return 2;
        case OctreeStats:
            return 1;
        case StopNode:
            return 1;
        case EntityAdd:
        case EntityEdit:
        case EntityData:
            return VERSION_ENTITIES_HAVE_SIMULATION_OWNER_AND_ACTIONS_OVER_WIRE;
        case EntityErase:
            return 2;
        case AudioStreamStats:
            return 1;
        case IceServerHeartbeat:
        case IceServerQuery:
            return 1;
        default:
            return 0;
    }
}

#define PACKET_TYPE_NAME_LOOKUP(x) case x:  return QString(#x);

QString nameForPacketType(PacketType::Value packetType) {
    switch (packetType) {
            PACKET_TYPE_NAME_LOOKUP(Unknown);
            PACKET_TYPE_NAME_LOOKUP(StunResponse);
            PACKET_TYPE_NAME_LOOKUP(DomainList);
            PACKET_TYPE_NAME_LOOKUP(Ping);
            PACKET_TYPE_NAME_LOOKUP(PingReply);
            PACKET_TYPE_NAME_LOOKUP(KillAvatar);
            PACKET_TYPE_NAME_LOOKUP(AvatarData);
            PACKET_TYPE_NAME_LOOKUP(InjectAudio);
            PACKET_TYPE_NAME_LOOKUP(MixedAudio);
            PACKET_TYPE_NAME_LOOKUP(MicrophoneAudioNoEcho);
            PACKET_TYPE_NAME_LOOKUP(MicrophoneAudioWithEcho);
            PACKET_TYPE_NAME_LOOKUP(BulkAvatarData);
            PACKET_TYPE_NAME_LOOKUP(SilentAudioFrame);
            PACKET_TYPE_NAME_LOOKUP(EnvironmentData);
            PACKET_TYPE_NAME_LOOKUP(DomainListRequest);
            PACKET_TYPE_NAME_LOOKUP(RequestAssignment);
            PACKET_TYPE_NAME_LOOKUP(CreateAssignment);
            PACKET_TYPE_NAME_LOOKUP(DomainConnectionDenied);
            PACKET_TYPE_NAME_LOOKUP(MuteEnvironment);
            PACKET_TYPE_NAME_LOOKUP(AudioStreamStats);
            PACKET_TYPE_NAME_LOOKUP(DataServerConfirm);
            PACKET_TYPE_NAME_LOOKUP(OctreeStats);
            PACKET_TYPE_NAME_LOOKUP(Jurisdiction);
            PACKET_TYPE_NAME_LOOKUP(JurisdictionRequest);
            PACKET_TYPE_NAME_LOOKUP(AvatarIdentity);
            PACKET_TYPE_NAME_LOOKUP(AvatarBillboard);
            PACKET_TYPE_NAME_LOOKUP(DomainConnectRequest);
            PACKET_TYPE_NAME_LOOKUP(DomainServerRequireDTLS);
            PACKET_TYPE_NAME_LOOKUP(NodeJsonStats);
            PACKET_TYPE_NAME_LOOKUP(EntityQuery);
            PACKET_TYPE_NAME_LOOKUP(EntityData);
            PACKET_TYPE_NAME_LOOKUP(EntityErase);
            PACKET_TYPE_NAME_LOOKUP(OctreeDataNack);
            PACKET_TYPE_NAME_LOOKUP(StopNode);
            PACKET_TYPE_NAME_LOOKUP(AudioEnvironment);
            PACKET_TYPE_NAME_LOOKUP(EntityEditNack);
            PACKET_TYPE_NAME_LOOKUP(SignedTransactionPayment);
            PACKET_TYPE_NAME_LOOKUP(IceServerHeartbeat);
            PACKET_TYPE_NAME_LOOKUP(DomainServerAddedNode);
            PACKET_TYPE_NAME_LOOKUP(IceServerQuery);
            PACKET_TYPE_NAME_LOOKUP(IceServerPeerInformation);
            PACKET_TYPE_NAME_LOOKUP(UnverifiedPing);
            PACKET_TYPE_NAME_LOOKUP(UnverifiedPingReply);
            PACKET_TYPE_NAME_LOOKUP(EntityAdd);
            PACKET_TYPE_NAME_LOOKUP(EntityEdit);
        default:
            return QString("Type: ") + QString::number((int)packetType);
    }
    return QString("unexpected");
}



QByteArray byteArrayWithUUIDPopulatedHeader(PacketType::Value packetType, const QUuid& connectionUUID) {
    QByteArray freshByteArray(MAX_PACKET_HEADER_BYTES, 0);
    freshByteArray.resize(populatePacketHeaderWithUUID(freshByteArray, packetType, connectionUUID));
    return freshByteArray;
}

int populatePacketHeaderWithUUID(QByteArray& packet, PacketType::Value packetType, const QUuid& connectionUUID) {
    if (packet.size() < numBytesForPacketHeaderGivenPacketType(packetType)) {
        packet.resize(numBytesForPacketHeaderGivenPacketType(packetType));
    }

    return populatePacketHeaderWithUUID(packet.data(), packetType, connectionUUID);
}

int populatePacketHeaderWithUUID(char* packet, PacketType::Value packetType, const QUuid& connectionUUID) {
    int numTypeBytes = packArithmeticallyCodedValue(packetType, packet);
    packet[numTypeBytes] = versionForPacketType(packetType);

    char* position = packet + numTypeBytes + sizeof(PacketVersion);

    QByteArray rfcUUID = connectionUUID.toRfc4122();
    memcpy(position, rfcUUID.constData(), NUM_BYTES_RFC4122_UUID);
    position += NUM_BYTES_RFC4122_UUID;

    if (!NON_VERIFIED_PACKETS.contains(packetType)) {
        // pack 16 bytes of zeros where the md5 hash will be placed once data is packed
        memset(position, 0, NUM_BYTES_MD5_HASH);
        position += NUM_BYTES_MD5_HASH;
    }

    if (SEQUENCE_NUMBERED_PACKETS.contains(packetType)) {
        // Pack zeros for the number of bytes that the sequence number requires.
        // The LimitedNodeList will handle packing in the sequence number when sending out the packet.
        memset(position, 0, sizeof(PacketSequenceNumber));
        position += sizeof(PacketSequenceNumber);
    }

    // return the number of bytes written for pointer pushing
    return position - packet;
}

int numBytesForPacketHeader(const QByteArray& packet) {
    PacketType::Value packetType = packetTypeForPacket(packet);
    return numBytesForPacketHeaderGivenPacketType(packetType);
}

int numBytesForPacketHeader(const char* packet) {
    PacketType::Value packetType = packetTypeForPacket(packet);
    return numBytesForPacketHeaderGivenPacketType(packetType);
}

int numBytesForArithmeticCodedPacketType(PacketType::Value packetType) {
    return (int) ceilf((float) packetType / 255);
}

int numBytesForPacketHeaderGivenPacketType(PacketType::Value packetType) {
    return numBytesForArithmeticCodedPacketType(packetType)
    + numHashBytesForType(packetType)
    + numSequenceNumberBytesForType(packetType)
    + NUM_STATIC_HEADER_BYTES;
}

int numHashBytesForType(PacketType::Value packetType) {
    return (NON_VERIFIED_PACKETS.contains(packetType) ? 0 : NUM_BYTES_MD5_HASH);
}

int numSequenceNumberBytesForType(PacketType::Value packetType) {
    return (SEQUENCE_NUMBERED_PACKETS.contains(packetType) ? sizeof(PacketSequenceNumber) : 0);
}

QUuid uuidFromPacketHeader(const QByteArray& packet) {
    return QUuid::fromRfc4122(packet.mid(numBytesArithmeticCodingFromBuffer(packet.data()) + sizeof(PacketVersion),
                                         NUM_BYTES_RFC4122_UUID));
}

int hashOffsetForPacketType(PacketType::Value packetType) {
    return numBytesForArithmeticCodedPacketType(packetType) + NUM_STATIC_HEADER_BYTES;
}

int sequenceNumberOffsetForPacketType(PacketType::Value packetType) {
    return numBytesForPacketHeaderGivenPacketType(packetType) - sizeof(PacketSequenceNumber);
}

QByteArray hashFromPacketHeader(const QByteArray& packet) {
    return packet.mid(hashOffsetForPacketType(packetTypeForPacket(packet)), NUM_BYTES_MD5_HASH);
}

QByteArray hashForPacketAndConnectionUUID(const QByteArray& packet, const QUuid& connectionUUID) {
    return QCryptographicHash::hash(packet.mid(numBytesForPacketHeader(packet)) + connectionUUID.toRfc4122(),
                                    QCryptographicHash::Md5);
}

PacketSequenceNumber sequenceNumberFromHeader(const QByteArray& packet, PacketType::Value packetType) {
    if (packetType == PacketType::Unknown) {
        packetType = packetTypeForPacket(packet);
    }

    PacketSequenceNumber result = DEFAULT_SEQUENCE_NUMBER;

    if (SEQUENCE_NUMBERED_PACKETS.contains(packetType)) {
        memcpy(&result, packet.data() + sequenceNumberOffsetForPacketType(packetType), sizeof(PacketSequenceNumber));
    }

    return result;
}

void replaceHashInPacket(QByteArray& packet, const QUuid& connectionUUID, PacketType::Value packetType) {
    if (packetType == PacketType::Unknown) {
        packetType = packetTypeForPacket(packet);
    }

    packet.replace(hashOffsetForPacketType(packetType), NUM_BYTES_MD5_HASH,
                   hashForPacketAndConnectionUUID(packet, connectionUUID));
}

void replaceSequenceNumberInPacket(QByteArray& packet, PacketSequenceNumber sequenceNumber, PacketType::Value packetType) {
    if (packetType == PacketType::Unknown) {
        packetType = packetTypeForPacket(packet);
    }

    packet.replace(sequenceNumberOffsetForPacketType(packetType),
                   sizeof(PacketSequenceNumber), reinterpret_cast<char*>(&sequenceNumber), sizeof(PacketSequenceNumber));
}

void replaceHashAndSequenceNumberInPacket(QByteArray& packet, const QUuid& connectionUUID, PacketSequenceNumber sequenceNumber,
                                          PacketType::Value packetType) {
    if (packetType == PacketType::Unknown) {
        packetType = packetTypeForPacket(packet);
    }

    replaceHashInPacket(packet, connectionUUID, packetType);
    replaceSequenceNumberInPacket(packet, sequenceNumber, packetType);
}

PacketType::Value packetTypeForPacket(const QByteArray& packet) {
    return (PacketType::Value) arithmeticCodingValueFromBuffer(packet.data());
}

PacketType::Value packetTypeForPacket(const char* packet) {
    return (PacketType::Value) arithmeticCodingValueFromBuffer(packet);
}
