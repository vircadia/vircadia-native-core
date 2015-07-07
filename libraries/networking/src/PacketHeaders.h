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

#pragma once

#include <cstdint>
#include <map>

#include <QtCore/QCryptographicHash>
#include <QtCore/QSet>
#include <QtCore/QUuid>

#include "UUID.h"

// NOTE: if adding a new packet packetType, you can replace one marked usable or add at the end
// NOTE: if you want the name of the packet packetType to be available for debugging or logging, update nameForPacketType() as well

namespace PacketType {
   enum Value {
        Unknown, // 0
        StunResponse,
        DomainList,
        Ping,
        PingReply,
        KillAvatar, // 5
        AvatarData,
        InjectAudio,
        MixedAudio,
        MicrophoneAudioNoEcho,
        MicrophoneAudioWithEcho, // 10
        BulkAvatarData,
        SilentAudioFrame,
        EnvironmentData,
        DomainListRequest,
        RequestAssignment, // 15
        CreateAssignment,
        DomainConnectionDenied,
        MuteEnvironment,
        AudioStreamStats,
        DataServerConfirm, // 20
        DomainServerPathQuery,
        DomainServerPathResponse,
        DomainServerAddedNode,
        IceServerPeerInformation,
        IceServerQuery, // 25
        OctreeStats,
        Jurisdiction,
        JurisdictionRequest,
        UNUSED_6,
        UNUSED_7, // 30
        UNUSED_8,
        UNUSED_9,
        NoisyMute,
        UNUSED_10,
        AvatarIdentity, // 35
        AvatarBillboard,
        DomainConnectRequest,
        DomainServerRequireDTLS,
        NodeJsonStats,
        EntityQuery, // 40
        EntityData,
        EntityAdd,
        EntityErase,
        EntityEdit,
        OctreeDataNack, // 45
        StopNode,
        AudioEnvironment,
        EntityEditNack,
        SignedTransactionPayment,
        ICEServerHeartbeat, // 50
        ICEPing,
        ICEPingReply
   };
};

typedef char PacketVersion;

typedef uint16_t PacketSequenceNumber;
const PacketSequenceNumber DEFAULT_SEQUENCE_NUMBER = 0;

typedef std::map<PacketType::Value, PacketSequenceNumber> PacketTypeSequenceMap;

extern const QSet<PacketType::Value> NON_VERIFIED_PACKETS;
extern const QSet<PacketType::Value> SEQUENCE_NUMBERED_PACKETS;
extern const QSet<PacketType::Value> NON_SOURCED_PACKETS;

const int NUM_BYTES_MD5_HASH = 16;
const int NUM_STATIC_HEADER_BYTES = sizeof(PacketVersion) + NUM_BYTES_RFC4122_UUID;
const int MAX_PACKET_HEADER_BYTES = sizeof(PacketType::Value) + NUM_BYTES_MD5_HASH + NUM_STATIC_HEADER_BYTES;

PacketType::Value packetTypeForPacket(const QByteArray& packet);
PacketType::Value packetTypeForPacket(const char* packet);

PacketVersion versionForPacketType(PacketType::Value packetType);
QString nameForPacketType(PacketType::Value packetType);

const QUuid nullUUID = QUuid();

QByteArray byteArrayWithUUIDPopulatedHeader(PacketType::Value packetType, const QUuid& connectionUUID);
int populatePacketHeaderWithUUID(QByteArray& packet, PacketType::Value packetType, const QUuid& connectionUUID);
int populatePacketHeaderWithUUID(char* packet, PacketType::Value packetType, const QUuid& connectionUUID);

int numHashBytesForType(PacketType::Value packetType);
int numSequenceNumberBytesForType(PacketType::Value packetType);

int numBytesForPacketHeader(const QByteArray& packet);
int numBytesForPacketHeader(const char* packet);
int numBytesForArithmeticCodedPacketType(PacketType::Value packetType);
int numBytesForPacketHeaderGivenPacketType(PacketType::Value packetType);
int packArithmeticallyCodedValue(int value, char* destination);

QUuid uuidFromPacketHeader(const QByteArray& packet);

int hashOffsetForPacketType(PacketType::Value packetType);
int sequenceNumberOffsetForPacketType(PacketType::Value packetType);

QByteArray hashFromPacketHeader(const QByteArray& packet);
QByteArray hashForPacketAndConnectionUUID(const QByteArray& packet, const QUuid& connectionUUID);

// NOTE: The following four methods accept a PacketType::Value which defaults to PacketType::Unknown.
// If the caller has already looked at the packet type and can provide it then the methods below won't have to look it up.

PacketSequenceNumber sequenceNumberFromHeader(const QByteArray& packet, PacketType::Value packetType = PacketType::Unknown);

void replaceHashInPacket(QByteArray& packet, const QUuid& connectionUUID, PacketType::Value packetType = PacketType::Unknown);

void replaceSequenceNumberInPacket(QByteArray& packet, PacketSequenceNumber sequenceNumber,
                                   PacketType::Value packetType = PacketType::Unknown);

void replaceHashAndSequenceNumberInPacket(QByteArray& packet, const QUuid& connectionUUID, PacketSequenceNumber sequenceNumber,
                                          PacketType::Value packetType = PacketType::Unknown);

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
const PacketVersion VERSION_ENTITIES_ZONE_ENTITIES_HAVE_SKYBOX = 21;
const PacketVersion VERSION_ENTITIES_ZONE_ENTITIES_STAGE_HAS_AUTOMATIC_HOURDAY = 22;
const PacketVersion VERSION_ENTITIES_PARTICLE_ENTITIES_HAVE_TEXTURES = 23;
const PacketVersion VERSION_ENTITIES_HAVE_LINE_TYPE = 24;
const PacketVersion VERSION_ENTITIES_HAVE_COLLISION_SOUND_URL = 25;
const PacketVersion VERSION_ENTITIES_HAVE_FRICTION = 26;
const PacketVersion VERSION_NO_ENTITY_ID_SWAP = 27;
const PacketVersion VERSION_ENTITIES_PARTICLE_FIX = 28;
const PacketVersion VERSION_ENTITIES_LINE_POINTS = 29;
const PacketVersion VERSION_ENTITIES_FACE_CAMERA = 30;
const PacketVersion VERSION_ENTITIES_SCRIPT_TIMESTAMP = 31;
const PacketVersion VERSION_ENTITIES_SCRIPT_TIMESTAMP_FIX = 32;
const PacketVersion VERSION_ENTITIES_HAVE_SIMULATION_OWNER_AND_ACTIONS_OVER_WIRE = 33;

#endif // hifi_PacketHeaders_h
