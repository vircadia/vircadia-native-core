//
//  PacketHeaders.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 4/8/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PacketHeaders_h
#define hifi_PacketHeaders_h

#include <QtCore/QCryptographicHash>
#include <QtCore/QSet>
#include <QtCore/QUuid>

#include "UUID.h"

// NOTE: if adding a new packet type, you can replace one marked usable or add at the end
// NOTE: if you want the name of the packet type to be available for debugging or logging, update nameForPacketType() as well
enum PacketType {
    PacketTypeUnknown, // 0
    PacketTypeStunResponse,
    PacketTypeDomainList,
    PacketTypePing,
    PacketTypePingReply,
    PacketTypeKillAvatar, // 5
    PacketTypeAvatarData,
    PacketTypeInjectAudio,
    PacketTypeMixedAudio,
    PacketTypeMicrophoneAudioNoEcho,
    PacketTypeMicrophoneAudioWithEcho, // 10
    PacketTypeBulkAvatarData,
    PacketTypeSilentAudioFrame,
    PacketTypeEnvironmentData,
    PacketTypeDomainListRequest,
    PacketTypeRequestAssignment, // 15
    PacketTypeCreateAssignment,
    PacketTypeDomainConnectionDenied,
    PacketTypeMuteEnvironment,
    PacketTypeAudioStreamStats,
    PacketTypeDataServerConfirm, // 20
    UNUSED_1,
    UNUSED_2,
    UNUSED_3,
    UNUSED_4,
    UNUSED_5, // 25
    PacketTypeOctreeStats,
    PacketTypeJurisdiction,
    PacketTypeJurisdictionRequest,
    UNUSED_6,
    UNUSED_7, // 30
    UNUSED_8,
    UNUSED_9,
    PacketTypeNoisyMute,
    UNUSED_10,
    PacketTypeAvatarIdentity, // 35
    PacketTypeAvatarBillboard,
    PacketTypeDomainConnectRequest,
    PacketTypeDomainServerRequireDTLS,
    PacketTypeNodeJsonStats,
    PacketTypeEntityQuery, // 40
    PacketTypeEntityData,
    PacketTypeEntityAddOrEdit,
    PacketTypeEntityErase,
    PacketTypeEntityAddResponse,
    PacketTypeOctreeDataNack, // 45
    PacketTypeStopNode,
    PacketTypeAudioEnvironment,
    PacketTypeEntityEditNack,
    PacketTypeSignedTransactionPayment,
    PacketTypeIceServerHeartbeat, // 50
    PacketTypeIceServerHeartbeatResponse,
    PacketTypeUnverifiedPing,
    PacketTypeUnverifiedPingReply
};

typedef char PacketVersion;

const QSet<PacketType> NON_VERIFIED_PACKETS = QSet<PacketType>()
    << PacketTypeDomainServerRequireDTLS << PacketTypeDomainConnectRequest
    << PacketTypeDomainList << PacketTypeDomainListRequest << PacketTypeDomainConnectionDenied
    << PacketTypeCreateAssignment << PacketTypeRequestAssignment << PacketTypeStunResponse
    << PacketTypeNodeJsonStats << PacketTypeEntityQuery
    << PacketTypeOctreeDataNack << PacketTypeEntityEditNack
    << PacketTypeIceServerHeartbeat << PacketTypeIceServerHeartbeatResponse
    << PacketTypeUnverifiedPing << PacketTypeUnverifiedPingReply << PacketTypeStopNode;

const int NUM_BYTES_MD5_HASH = 16;
const int NUM_STATIC_HEADER_BYTES = sizeof(PacketVersion) + NUM_BYTES_RFC4122_UUID;
const int MAX_PACKET_HEADER_BYTES = sizeof(PacketType) + NUM_BYTES_MD5_HASH + NUM_STATIC_HEADER_BYTES;

PacketVersion versionForPacketType(PacketType type);
QString nameForPacketType(PacketType type);

const QUuid nullUUID = QUuid();

QByteArray byteArrayWithPopulatedHeader(PacketType type, const QUuid& connectionUUID = nullUUID);
int populatePacketHeader(QByteArray& packet, PacketType type, const QUuid& connectionUUID = nullUUID);
int populatePacketHeader(char* packet, PacketType type, const QUuid& connectionUUID = nullUUID);

int numHashBytesInPacketHeaderGivenPacketType(PacketType type);

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

const PacketVersion VERSION_OCTREE_HAS_FILE_BREAKS = 1;
const PacketVersion VERSION_ENTITIES_HAVE_ANIMATION = 1;
const PacketVersion VERSION_ROOT_ELEMENT_HAS_DATA = 2;
const PacketVersion VERSION_ENTITIES_SUPPORT_SPLIT_MTU = 3;
const PacketVersion VERSION_ENTITIES_HAS_FILE_BREAKS = VERSION_ENTITIES_SUPPORT_SPLIT_MTU;
const PacketVersion VERSION_ENTITIES_SUPPORT_DIMENSIONS = 4;
const PacketVersion VERSION_ENTITIES_MODELS_HAVE_ANIMATION_SETTINGS = 5;
const PacketVersion VERSION_ENTITIES_HAVE_USER_DATA = 6;
const PacketVersion VERSION_ENTITIES_HAS_LAST_SIMULATED_TIME = 7;
const PacketVersion VERSION_MODEL_ENTITIES_SUPPORT_SHAPE_TYPE = 8;
const PacketVersion VERSION_ENTITIES_LIGHT_HAS_INTENSITY_AND_COLOR_PROPERTIES = 9;
const PacketVersion VERSION_ENTITIES_HAS_PARTICLES = 10;
const PacketVersion VERSION_ENTITIES_USE_METERS_AND_RADIANS = 11;
const PacketVersion VERSION_ENTITIES_HAS_COLLISION_MODEL = 12;
const PacketVersion VERSION_ENTITIES_HAS_MARKETPLACE_ID_DAMAGED = 13;
const PacketVersion VERSION_ENTITIES_HAS_MARKETPLACE_ID = 14;
const PacketVersion VERSION_ENTITIES_HAVE_ACCELERATION = 15;
const PacketVersion VERSION_ENTITIES_HAVE_UUIDS = 16;
const PacketVersion VERSION_ENTITIES_ZONE_ENTITIES_EXIST = 17;
const PacketVersion VERSION_ENTITIES_ZONE_ENTITIES_HAVE_DYNAMIC_SHAPE = 18;
const PacketVersion VERSION_ENTITIES_HAVE_NAMES = 19;
const PacketVersion VERSION_ENTITIES_ZONE_ENTITIES_HAVE_ATMOSPHERE = 20;

#endif // hifi_PacketHeaders_h
