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

#include <QtCore/QCryptographicHash>
#include <QtCore/QUuid>

#include "UUID.h"

// NOTE: if adding a new packet type, you can replace one marked usable or add at the end

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
    PacketTypeTransmitterData, // usable
    PacketTypeEnvironmentData,
    PacketTypeDomainListRequest,
    PacketTypeRequestAssignment,
    PacketTypeCreateAssignment,
    PacketTypeDataServerPut,
    PacketTypeDataServerGet,
    PacketTypeDataServerHashPut,
    PacketTypeDataServerHashGet,
    PacketTypeDataServerHashSend,
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
    PacketTypeAvatarIdentity,
    PacketTypeAvatarBillboard,
    PacketTypeDomainConnectRequest,
    PacketTypeDomainServerAuthRequest
};

typedef char PacketVersion;

const int NUM_BYTES_MD5_HASH = 16;
const int NUM_STATIC_HEADER_BYTES = sizeof(PacketVersion) + NUM_BYTES_RFC4122_UUID + NUM_BYTES_MD5_HASH;
const int MAX_PACKET_HEADER_BYTES = sizeof(PacketType) + NUM_STATIC_HEADER_BYTES;

PacketVersion versionForPacketType(PacketType type);

const QUuid nullUUID = QUuid();

QByteArray byteArrayWithPopulatedHeader(PacketType type, const QUuid& connectionUUID = nullUUID);
int populatePacketHeader(QByteArray& packet, PacketType type, const QUuid& connectionUUID = nullUUID);
int populatePacketHeader(char* packet, PacketType type, const QUuid& connectionUUID = nullUUID);

int numBytesForPacketHeader(const QByteArray& packet);
int numBytesForPacketHeader(const char* packet);
int numBytesForPacketHeaderGivenPacketType(PacketType type);

QUuid uuidFromPacketHeader(const QByteArray& packet);

QByteArray hashFromPacketHeader(const QByteArray& packet);
QByteArray hashForPacketAndConnectionUUID(const QByteArray& packet, const QUuid& connectionUUID);
void replaceHashInPacketGivenConnectionUUID(QByteArray& packet, const QUuid& connectionUUID);

PacketType packetTypeForPacket(const QByteArray& packet);
PacketType packetTypeForPacket(const char* packet);

int arithmeticCodingValueFromBuffer(const char* checkValue);
int numBytesArithmeticCodingFromBuffer(const char* checkValue);

#endif
