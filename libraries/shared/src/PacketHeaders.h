//
//  PacketHeaders.h
//  hifi
//
//  Created by Stephen Birarda on 4/8/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  The packet headers below refer to the first byte of a received UDP packet transmitted between
//  any two Hifi components.  For example, a packet whose first byte is 'P' is always a ping packet.
//  

#ifndef hifi_PacketHeaders_h
#define hifi_PacketHeaders_h

#include <QtCore/QUuid>

#include "UUID.h"

enum PacketType {
    PacketTypeUnknown,
    PacketTypeStunResponse,
    PacketTypeDomainList,
    PacketTypePing,
    PacketTypePingReply,
    PacketTypeKillAvatar,
    PacketTypeAvatarData,
    PacketTypeInjectAudio,
    PacketTypeMixedAudio,
    PacketTypeMicrophoneAudioNoEcho,
    PacketTypeMicrophoneAudioWithEcho,
    PacketTypeBulkAvatarData,
    PacketTypeTransmitterData,
    PacketTypeEnvironmentData,
    PacketTypeDomainListRequest,
    PacketTypeRequestAssignment,
    PacketTypeCreateAssignment,
    PacketTypeDataServerPut,
    PacketTypeDataServerGet,
    PacketTypeDataServerSend,
    PacketTypeDataServerConfirm,
    PacketTypeVoxelQuery,
    PacketTypeVoxelData,
    PacketTypeVoxelSet,
    PacketTypeVoxelSetDestructive,
    PacketTypeVoxelErase,
    PacketTypeOctreeStats,
    PacketTypeJurisdiction,
    PacketTypeJurisdictionRequest,
    PacketTypeParticleQuery,
    PacketTypeParticleData,
    PacketTypeParticleAddOrEdit,
    PacketTypeParticleErase,
    PacketTypeParticleAddResponse,
    PacketTypeMetavoxelData,
    PacketTypeAvatarIdentity
};

typedef char PacketVersion;

const int MAX_PACKET_HEADER_BYTES = sizeof(PacketType) + sizeof(PacketVersion) + NUM_BYTES_RFC4122_UUID;;

PacketVersion versionForPacketType(PacketType type);

const QUuid nullUUID = QUuid();

QByteArray byteArrayWithPopluatedHeader(PacketType type, const QUuid& connectionUUID = nullUUID);
int populatePacketHeader(QByteArray& packet, PacketType type, const QUuid& connectionUUID = nullUUID);
int populatePacketHeader(char* packet, PacketType type, const QUuid& connectionUUID = nullUUID);

bool packetVersionMatch(const QByteArray& packet);

int numBytesForPacketHeader(const QByteArray& packet);
int numBytesForPacketHeader(const char* packet);
int numBytesForPacketHeaderGivenPacketType(PacketType type);

void deconstructPacketHeader(const QByteArray& packet, QUuid& senderUUID);

PacketType packetTypeForPacket(const QByteArray& packet);
PacketType packetTypeForPacket(const char* packet);

int arithmeticCodingValueFromBuffer(const char* checkValue);

#endif
