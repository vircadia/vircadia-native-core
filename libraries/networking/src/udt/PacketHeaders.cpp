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
#include <QtCore/QMetaEnum>


Q_DECLARE_METATYPE(PacketType);
static int packetTypeMetaTypeId = qRegisterMetaType<PacketType>();

const QSet<PacketType> NON_VERIFIED_PACKETS = QSet<PacketType>()
    << PacketType::NodeJsonStats << PacketType::EntityQuery
    << PacketType::OctreeDataNack << PacketType::EntityEditNack
    << PacketType::DomainListRequest << PacketType::StopNode
    << PacketType::DomainDisconnectRequest;

const QSet<PacketType> NON_SOURCED_PACKETS = QSet<PacketType>()
    << PacketType::StunResponse << PacketType::CreateAssignment << PacketType::RequestAssignment
    << PacketType::DomainServerRequireDTLS << PacketType::DomainConnectRequest
    << PacketType::DomainList << PacketType::DomainConnectionDenied
    << PacketType::DomainServerPathQuery << PacketType::DomainServerPathResponse
    << PacketType::DomainServerAddedNode << PacketType::DomainServerConnectionToken
    << PacketType::DomainSettingsRequest << PacketType::DomainSettings
    << PacketType::ICEServerPeerInformation << PacketType::ICEServerQuery << PacketType::ICEServerHeartbeat
    << PacketType::ICEPing << PacketType::ICEPingReply << PacketType::ICEServerHeartbeatDenied
    << PacketType::AssignmentClientStatus << PacketType::StopNode
    << PacketType::DomainServerRemovedNode;

const QSet<PacketType> RELIABLE_PACKETS = QSet<PacketType>();

PacketVersion versionForPacketType(PacketType packetType) {
    switch (packetType) {
        case PacketType::DomainList:
            return 18;
        case PacketType::EntityAdd:
        case PacketType::EntityEdit:
        case PacketType::EntityData:
            return VERSION_LIGHT_HAS_FALLOFF_RADIUS;
        case PacketType::AvatarData:
        case PacketType::BulkAvatarData:
            return static_cast<PacketVersion>(AvatarMixerPacketVersion::SoftAttachmentSupport);
        case PacketType::ICEServerHeartbeat:
            return 18; // ICE Server Heartbeat signing
        case PacketType::AssetGetInfo:
        case PacketType::AssetGet:
        case PacketType::AssetUpload:
            // Removal of extension from Asset requests
            return 18;
        default:
            return 17;
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
