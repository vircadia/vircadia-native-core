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
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QUuid>

// The enums are inside this PacketTypeEnum for run-time conversion of enum value to string via
// Q_ENUMS, without requiring a macro that is called for each enum value.
class PacketTypeEnum {
    Q_GADGET
    Q_ENUMS(Value)
public:
    // If adding a new packet packetType, you can replace one marked usable or add at the end.
    // This enum must hold 256 or fewer packet types (so the value is <= 255) since it is statically typed as a uint8_t
    enum class Value : uint8_t {
        Unknown,
        StunResponse,
        DomainList,
        Ping,
        PingReply,
        KillAvatar,
        AvatarData,
        InjectAudio,
        MixedAudio,
        MicrophoneAudioNoEcho,
        MicrophoneAudioWithEcho,
        BulkAvatarData,
        SilentAudioFrame,
        DomainListRequest,
        RequestAssignment,
        CreateAssignment,
        DomainConnectionDenied,
        MuteEnvironment,
        AudioStreamStats,
        DomainServerPathQuery,
        DomainServerPathResponse,
        DomainServerAddedNode,
        ICEServerPeerInformation,
        ICEServerQuery,
        OctreeStats,
        Jurisdiction,
        JurisdictionRequest,
        AssignmentClientStatus,
        NoisyMute,
        AvatarIdentity,
        NodeIgnoreRequest,
        DomainConnectRequest,
        DomainServerRequireDTLS,
        NodeJsonStats,
        OctreeDataNack,
        StopNode,
        AudioEnvironment,
        EntityEditNack,
        ICEServerHeartbeat,
        ICEPing,
        ICEPingReply,
        EntityData,
        EntityQuery,
        EntityAdd,
        EntityErase,
        EntityEdit,
        DomainServerConnectionToken,
        DomainSettingsRequest,
        DomainSettings,
        AssetGet,
        AssetGetReply,
        AssetUpload,
        AssetUploadReply,
        AssetGetInfo,
        AssetGetInfoReply,
        DomainDisconnectRequest,
        DomainServerRemovedNode,
        MessagesData,
        MessagesSubscribe,
        MessagesUnsubscribe,
        ICEServerHeartbeatDenied,
        AssetMappingOperation,
        AssetMappingOperationReply,
        ICEServerHeartbeatACK,
        NegotiateAudioFormat,
        SelectedAudioFormat,
        MoreEntityShapes,
        NodeKickRequest,
        NodeMuteRequest,
        RadiusIgnoreRequest,
        UsernameFromIDRequest,
        UsernameFromIDReply,
        ViewFrustum,
        RequestsDomainListData,
        PerAvatarGainSet,
        EntityScriptGetStatus,
        EntityScriptGetStatusReply,
        ReloadEntityServerScript,
        EntityPhysics,
        EntityServerScriptLog,
        AdjustAvatarSorting,
        OctreeFileReplacement,
        CollisionEventChanges,
        ReplicatedMicrophoneAudioNoEcho,
        ReplicatedMicrophoneAudioWithEcho,
        ReplicatedInjectAudio,
        ReplicatedSilentAudioFrame,
        ReplicatedAvatarIdentity,
        ReplicatedKillAvatar,
        ReplicatedBulkAvatarData,
        OctreeFileReplacementFromUrl,
        ChallengeOwnership,
        EntityScriptCallMethod,
        ChallengeOwnershipRequest,
        ChallengeOwnershipReply,
        NUM_PACKET_TYPE
    };

    const static QHash<PacketTypeEnum::Value, PacketTypeEnum::Value> getReplicatedPacketMapping() {
        const static QHash<PacketTypeEnum::Value, PacketTypeEnum::Value> REPLICATED_PACKET_MAPPING {
            { PacketTypeEnum::Value::MicrophoneAudioNoEcho, PacketTypeEnum::Value::ReplicatedMicrophoneAudioNoEcho },
            { PacketTypeEnum::Value::MicrophoneAudioWithEcho, PacketTypeEnum::Value::ReplicatedMicrophoneAudioWithEcho },
            { PacketTypeEnum::Value::InjectAudio, PacketTypeEnum::Value::ReplicatedInjectAudio },
            { PacketTypeEnum::Value::SilentAudioFrame, PacketTypeEnum::Value::ReplicatedSilentAudioFrame },
            { PacketTypeEnum::Value::AvatarIdentity, PacketTypeEnum::Value::ReplicatedAvatarIdentity },
            { PacketTypeEnum::Value::KillAvatar, PacketTypeEnum::Value::ReplicatedKillAvatar },
            { PacketTypeEnum::Value::BulkAvatarData, PacketTypeEnum::Value::ReplicatedBulkAvatarData }
        };
        return REPLICATED_PACKET_MAPPING;
    }

    const static QSet<PacketTypeEnum::Value> getNonVerifiedPackets() {
        const static QSet<PacketTypeEnum::Value> NON_VERIFIED_PACKETS = QSet<PacketTypeEnum::Value>()
            << PacketTypeEnum::Value::NodeJsonStats
            << PacketTypeEnum::Value::EntityQuery
            << PacketTypeEnum::Value::OctreeDataNack
            << PacketTypeEnum::Value::EntityEditNack
            << PacketTypeEnum::Value::DomainListRequest
            << PacketTypeEnum::Value::StopNode
            << PacketTypeEnum::Value::DomainDisconnectRequest
            << PacketTypeEnum::Value::UsernameFromIDRequest
            << PacketTypeEnum::Value::NodeKickRequest
            << PacketTypeEnum::Value::NodeMuteRequest;
        return NON_VERIFIED_PACKETS;
    }

    const static QSet<PacketTypeEnum::Value> getNonSourcedPackets() {
        const static QSet<PacketTypeEnum::Value> NON_SOURCED_PACKETS = QSet<PacketTypeEnum::Value>()
            << PacketTypeEnum::Value::StunResponse << PacketTypeEnum::Value::CreateAssignment
            << PacketTypeEnum::Value::RequestAssignment << PacketTypeEnum::Value::DomainServerRequireDTLS
            << PacketTypeEnum::Value::DomainConnectRequest << PacketTypeEnum::Value::DomainList
            << PacketTypeEnum::Value::DomainConnectionDenied << PacketTypeEnum::Value::DomainServerPathQuery
            << PacketTypeEnum::Value::DomainServerPathResponse << PacketTypeEnum::Value::DomainServerAddedNode
            << PacketTypeEnum::Value::DomainServerConnectionToken << PacketTypeEnum::Value::DomainSettingsRequest
            << PacketTypeEnum::Value::DomainSettings << PacketTypeEnum::Value::ICEServerPeerInformation
            << PacketTypeEnum::Value::ICEServerQuery << PacketTypeEnum::Value::ICEServerHeartbeat
            << PacketTypeEnum::Value::ICEServerHeartbeatACK << PacketTypeEnum::Value::ICEPing
            << PacketTypeEnum::Value::ICEPingReply << PacketTypeEnum::Value::ICEServerHeartbeatDenied
            << PacketTypeEnum::Value::AssignmentClientStatus << PacketTypeEnum::Value::StopNode
            << PacketTypeEnum::Value::DomainServerRemovedNode << PacketTypeEnum::Value::UsernameFromIDReply
            << PacketTypeEnum::Value::OctreeFileReplacement << PacketTypeEnum::Value::ReplicatedMicrophoneAudioNoEcho
            << PacketTypeEnum::Value::ReplicatedMicrophoneAudioWithEcho << PacketTypeEnum::Value::ReplicatedInjectAudio
            << PacketTypeEnum::Value::ReplicatedSilentAudioFrame << PacketTypeEnum::Value::ReplicatedAvatarIdentity
            << PacketTypeEnum::Value::ReplicatedKillAvatar << PacketTypeEnum::Value::ReplicatedBulkAvatarData;
        return NON_SOURCED_PACKETS;
    }
};

using PacketType = PacketTypeEnum::Value;

const int NUM_BYTES_MD5_HASH = 16;

typedef char PacketVersion;

PacketVersion versionForPacketType(PacketType packetType);
QByteArray protocolVersionsSignature(); /// returns a unqiue signature for all the current protocols
QString protocolVersionsSignatureBase64();

#if (PR_BUILD || DEV_BUILD)
void sendWrongProtocolVersionsSignature(bool sendWrongVersion); /// for debugging version negotiation
#endif

uint qHash(const PacketType& key, uint seed);
QDebug operator<<(QDebug debug, const PacketType& type);

enum class EntityVersion : PacketVersion {
    StrokeColorProperty = 77,
    HasDynamicOwnershipTests,
    HazeEffect,
    StaticCertJsonVersionOne
};

enum class EntityScriptCallMethodVersion : PacketVersion {
    ServerCallable = 18,
    ClientCallable = 19
};

enum class EntityQueryPacketVersion: PacketVersion {
    JSONFilter = 18,
    JSONFilterWithFamilyTree = 19,
    ConnectionIdentifier = 20
};

enum class AssetServerPacketVersion: PacketVersion {
    VegasCongestionControl = 19,
    RangeRequestSupport,
    RedirectedMappings
};

enum class AvatarMixerPacketVersion : PacketVersion {
    TranslationSupport = 17,
    SoftAttachmentSupport,
    AvatarEntities,
    AbsoluteSixByteRotations,
    SensorToWorldMat,
    HandControllerJoints,
    HasKillAvatarReason,
    SessionDisplayName,
    Unignore,
    ImmediateSessionDisplayNameUpdates,
    VariableAvatarData,
    AvatarAsChildFixes,
    StickAndBallDefaultAvatar,
    IdentityPacketsIncludeUpdateTime,
    AvatarIdentitySequenceId,
    MannequinDefaultAvatar,
    AvatarIdentitySequenceFront,
    IsReplicatedInAvatarIdentity,
    AvatarIdentityLookAtSnapping,
    UpdatedMannequinDefaultAvatar
};

enum class DomainConnectRequestVersion : PacketVersion {
    NoHostname = 17,
    HasHostname,
    HasProtocolVersions,
    HasMACAddress,
    HasMachineFingerprint,
    AlwaysHasMachineFingerprint
};

enum class DomainConnectionDeniedVersion : PacketVersion {
    ReasonMessageOnly = 17,
    IncludesReasonCode,
    IncludesExtraInfo
};

enum class DomainServerAddedNodeVersion : PacketVersion {
    PrePermissionsGrid = 17,
    PermissionsGrid
};

enum class DomainListVersion : PacketVersion {
    PrePermissionsGrid = 18,
    PermissionsGrid,
    GetUsernameFromUUIDSupport,
    GetMachineFingerprintFromUUIDSupport
};

enum class AudioVersion : PacketVersion {
    HasCompressedAudio = 17,
    CodecNameInAudioPackets,
    Exactly10msAudioPackets,
    TerminatingStreamStats,
    SpaceBubbleChanges,
    HasPersonalMute,
    HighDynamicRangeVolume,
};

enum class MessageDataVersion : PacketVersion {
    TextOrBinaryData = 18
};

enum class IcePingVersion : PacketVersion {
    SendICEPeerID = 18
};

#endif // hifi_PacketHeaders_h
