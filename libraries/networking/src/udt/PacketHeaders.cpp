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

const QSet<PacketType> NON_VERIFIED_PACKETS = QSet<PacketType>()
    << PacketType::NodeJsonStats << PacketType::EntityQuery
    << PacketType::OctreeDataNack << PacketType::EntityEditNack
    << PacketType::DomainListRequest << PacketType::StopNode;

const QSet<PacketType> NON_SOURCED_PACKETS = QSet<PacketType>()
    << PacketType::StunResponse << PacketType::CreateAssignment << PacketType::RequestAssignment
    << PacketType::DomainServerRequireDTLS << PacketType::DomainConnectRequest
    << PacketType::DomainList << PacketType::DomainConnectionDenied
    << PacketType::DomainServerPathQuery << PacketType::DomainServerPathResponse
    << PacketType::DomainServerAddedNode
    << PacketType::DomainSettingsRequest << PacketType::DomainSettings
    << PacketType::ICEServerPeerInformation << PacketType::ICEServerQuery << PacketType::ICEServerHeartbeat
    << PacketType::ICEPing << PacketType::ICEPingReply
    << PacketType::AssignmentClientStatus << PacketType::StopNode;

const QSet<PacketType> RELIABLE_PACKETS = QSet<PacketType>();

PacketVersion versionForPacketType(PacketType packetType) {
    switch (packetType) {
        case PacketType::EntityAdd:
        case PacketType::EntityEdit:
        case PacketType::EntityData:
            return VERSION_ENTITIES_PROTOCOL_HEADER_SWAP;
        default:
            return 13;
    }
}

#define PACKET_TYPE_NAME_LOOKUP(x) case x:  return QString(#x);

QString nameForPacketType(PacketType packetType) {
    switch (packetType) {
        PACKET_TYPE_NAME_LOOKUP(PacketType::Unknown);
        PACKET_TYPE_NAME_LOOKUP(PacketType::StunResponse);
        PACKET_TYPE_NAME_LOOKUP(PacketType::DomainList);
        PACKET_TYPE_NAME_LOOKUP(PacketType::Ping);
        PACKET_TYPE_NAME_LOOKUP(PacketType::PingReply);
        PACKET_TYPE_NAME_LOOKUP(PacketType::KillAvatar);
        PACKET_TYPE_NAME_LOOKUP(PacketType::AvatarData);
        PACKET_TYPE_NAME_LOOKUP(PacketType::InjectAudio);
        PACKET_TYPE_NAME_LOOKUP(PacketType::MixedAudio);
        PACKET_TYPE_NAME_LOOKUP(PacketType::MicrophoneAudioNoEcho);
        PACKET_TYPE_NAME_LOOKUP(PacketType::MicrophoneAudioWithEcho);
        PACKET_TYPE_NAME_LOOKUP(PacketType::BulkAvatarData);
        PACKET_TYPE_NAME_LOOKUP(PacketType::SilentAudioFrame);
        PACKET_TYPE_NAME_LOOKUP(PacketType::DomainListRequest);
        PACKET_TYPE_NAME_LOOKUP(PacketType::RequestAssignment);
        PACKET_TYPE_NAME_LOOKUP(PacketType::CreateAssignment);
        PACKET_TYPE_NAME_LOOKUP(PacketType::DomainConnectionDenied);
        PACKET_TYPE_NAME_LOOKUP(PacketType::MuteEnvironment);
        PACKET_TYPE_NAME_LOOKUP(PacketType::AudioStreamStats);
        PACKET_TYPE_NAME_LOOKUP(PacketType::OctreeStats);
        PACKET_TYPE_NAME_LOOKUP(PacketType::Jurisdiction);
        PACKET_TYPE_NAME_LOOKUP(PacketType::JurisdictionRequest);
        PACKET_TYPE_NAME_LOOKUP(PacketType::AvatarIdentity);
        PACKET_TYPE_NAME_LOOKUP(PacketType::AvatarBillboard);
        PACKET_TYPE_NAME_LOOKUP(PacketType::DomainConnectRequest);
        PACKET_TYPE_NAME_LOOKUP(PacketType::DomainServerRequireDTLS);
        PACKET_TYPE_NAME_LOOKUP(PacketType::NodeJsonStats);
        PACKET_TYPE_NAME_LOOKUP(PacketType::EntityQuery);
        PACKET_TYPE_NAME_LOOKUP(PacketType::EntityData);
        PACKET_TYPE_NAME_LOOKUP(PacketType::EntityErase);
        PACKET_TYPE_NAME_LOOKUP(PacketType::OctreeDataNack);
        PACKET_TYPE_NAME_LOOKUP(PacketType::StopNode);
        PACKET_TYPE_NAME_LOOKUP(PacketType::AudioEnvironment);
        PACKET_TYPE_NAME_LOOKUP(PacketType::EntityEditNack);
        PACKET_TYPE_NAME_LOOKUP(PacketType::ICEServerHeartbeat);
        PACKET_TYPE_NAME_LOOKUP(PacketType::DomainServerAddedNode);
        PACKET_TYPE_NAME_LOOKUP(PacketType::ICEServerQuery);
        PACKET_TYPE_NAME_LOOKUP(PacketType::ICEServerPeerInformation);
        PACKET_TYPE_NAME_LOOKUP(PacketType::ICEPing);
        PACKET_TYPE_NAME_LOOKUP(PacketType::ICEPingReply);
        PACKET_TYPE_NAME_LOOKUP(PacketType::EntityAdd);
        PACKET_TYPE_NAME_LOOKUP(PacketType::EntityEdit);
            PACKET_TYPE_NAME_LOOKUP(PacketType::DomainServerConnectionToken);
        default:
            return QString("Type: ") + QString::number((int)packetType);
    }
    return QString("unexpected");
}

uint qHash(const PacketType& key, uint seed) {
    // seems odd that Qt couldn't figure out this cast itself, but this fixes a compile error after switch to
    // strongly typed enum for PacketType
    return qHash((quint8) key, seed);
}

QDebug operator<<(QDebug debug, const PacketType& type) {
    debug.nospace() << (uint8_t) type << " (" << qPrintable(nameForPacketType(type)) << ")";
    return debug.space();
}
