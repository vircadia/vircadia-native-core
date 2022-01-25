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
#include <mutex>

#include <QtCore/QDataStream>
#include <QtCore/QDebug>
#include <QtCore/QMetaEnum>


Q_DECLARE_METATYPE(PacketType);
int packetTypeMetaTypeId = qRegisterMetaType<PacketType>();

PacketVersion versionForPacketType(PacketType packetType) {
    switch (packetType) {
        case PacketType::DomainConnectRequestPending: // keeping the old version to maintain the protocol hash
            return 17;
        case PacketType::DomainList:
            return static_cast<PacketVersion>(DomainListVersion::SocketTypes);
        case PacketType::EntityAdd:
        case PacketType::EntityClone:
        case PacketType::EntityEdit:
        case PacketType::EntityData:
        case PacketType::EntityPhysics:
            return static_cast<PacketVersion>(EntityVersion::LAST_PACKET_TYPE);
        case PacketType::EntityQuery:
            return static_cast<PacketVersion>(EntityQueryPacketVersion::ConicalFrustums);
        case PacketType::AvatarIdentity:
        case PacketType::AvatarData:
            return static_cast<PacketVersion>(AvatarMixerPacketVersion::ARKitBlendshapes);
        case PacketType::BulkAvatarData:
        case PacketType::KillAvatar:
            return static_cast<PacketVersion>(AvatarMixerPacketVersion::ARKitBlendshapes);
        case PacketType::MessagesData:
            return static_cast<PacketVersion>(MessageDataVersion::TextOrBinaryData);
        // ICE packets
        case PacketType::ICEServerPeerInformation:
            return 17;
        case PacketType::ICEServerHeartbeatACK:
            return 17;
        case PacketType::ICEServerQuery:
            return 17;
        case PacketType::ICEServerHeartbeat:
            return 18; // ICE Server Heartbeat signing
        case PacketType::ICEPing:
            return static_cast<PacketVersion>(IcePingVersion::SendICEPeerID);
        case PacketType::ICEPingReply:
            return 17;
        case PacketType::ICEServerHeartbeatDenied:
            return 17;
        case PacketType::AssetMappingOperation:
        case PacketType::AssetMappingOperationReply:
        case PacketType::AssetGetInfo:
        case PacketType::AssetGet:
        case PacketType::AssetUpload:
            return static_cast<PacketVersion>(AssetServerPacketVersion::BakingTextureMeta);
        case PacketType::NodeIgnoreRequest:
            return 18; // Introduction of node ignore request (which replaced an unused packet tpye)

        case PacketType::DomainConnectionDenied:
            return static_cast<PacketVersion>(DomainConnectionDeniedVersion::IncludesExtraInfo);

        case PacketType::DomainConnectRequest:
            return static_cast<PacketVersion>(DomainConnectRequestVersion::SocketTypes);
        case PacketType::DomainListRequest:
            return static_cast<PacketVersion>(DomainListRequestVersion::SocketTypes);

        case PacketType::DomainServerAddedNode:
            return static_cast<PacketVersion>(DomainServerAddedNodeVersion::SocketTypes);

        case PacketType::EntityScriptCallMethod:
            return static_cast<PacketVersion>(EntityScriptCallMethodVersion::ClientCallable);

        case PacketType::MixedAudio:
        case PacketType::SilentAudioFrame:
        case PacketType::InjectAudio:
        case PacketType::MicrophoneAudioNoEcho:
        case PacketType::MicrophoneAudioWithEcho:
        case PacketType::AudioStreamStats:
        case PacketType::StopInjector:
            return static_cast<PacketVersion>(AudioVersion::StopInjectors);
        case PacketType::DomainSettings:
            return 18;  // replace min_avatar_scale and max_avatar_scale with min_avatar_height and max_avatar_height
        case PacketType::Ping:
            return static_cast<PacketVersion>(PingVersion::IncludeConnectionID);
        case PacketType::AvatarQuery:
            return static_cast<PacketVersion>(AvatarQueryVersion::ConicalFrustums);
        case PacketType::EntityQueryInitialResultsComplete:
            return static_cast<PacketVersion>(EntityVersion::ParticleSpin);
        case PacketType::BulkAvatarTraitsAck:
        case PacketType::BulkAvatarTraits:
            return static_cast<PacketVersion>(AvatarMixerPacketVersion::AvatarTraitsAck);
        default:
            return 22;
    }
}

uint qHash(const PacketType& key, uint seed) {
    // seems odd that Qt couldn't figure out this cast itself, but this fixes a compile error after switch
    // to strongly typed enum for PacketType
    return qHash((quint8) key, seed);
}

QDebug operator<<(QDebug debug, const PacketType& type) {
    QMetaObject metaObject = PacketTypeEnum::staticMetaObject;
    QMetaEnum metaEnum = metaObject.enumerator(metaObject.enumeratorOffset());
    QString typeName = metaEnum.valueToKey((int) type);

    debug.nospace().noquote() << (uint8_t) type << " (" << typeName << ")";
    return debug.space();
}

#if (PR_BUILD || DEV_BUILD)
static bool sendWrongProtocolVersion = false;
void sendWrongProtocolVersionsSignature(bool sendWrongVersion) {
    sendWrongProtocolVersion = sendWrongVersion;
}
#endif

static QByteArray protocolVersionSignature;
static QString protocolVersionSignatureBase64;
static void ensureProtocolVersionsSignature() {
    static std::once_flag once;
    std::call_once(once, [&] {
        QByteArray buffer;
        QDataStream stream(&buffer, QIODevice::WriteOnly);
        uint8_t numberOfProtocols = static_cast<uint8_t>(PacketType::NUM_PACKET_TYPE);
        stream << numberOfProtocols;
        for (uint8_t packetType = 0; packetType < numberOfProtocols; packetType++) {
            uint8_t packetTypeVersion = static_cast<uint8_t>(versionForPacketType(static_cast<PacketType>(packetType)));
            stream << packetTypeVersion;
        }
        QCryptographicHash hash(QCryptographicHash::Md5);
        hash.addData(buffer);
        protocolVersionSignature = hash.result();
        protocolVersionSignatureBase64 = protocolVersionSignature.toBase64();
    });
}
QByteArray protocolVersionsSignature() {
    ensureProtocolVersionsSignature();
    #if (PR_BUILD || DEV_BUILD)
    if (sendWrongProtocolVersion) {
        return QByteArray("INCORRECTVERSION"); // only for debugging version checking
    }
    #endif

    return protocolVersionSignature;
}
QString protocolVersionsSignatureBase64() {
    ensureProtocolVersionsSignature();
    return protocolVersionSignatureBase64;
}
