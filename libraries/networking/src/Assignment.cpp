//
//  Assignment.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PacketHeaders.h"
#include "SharedUtil.h"
#include "UUID.h"

#include <QtCore/QDataStream>

#include "Assignment.h"

Assignment::Type Assignment::typeForNodeType(NodeType_t nodeType) {
    switch (nodeType) {
        case NodeType::AudioMixer:
            return Assignment::AudioMixerType;
        case NodeType::AvatarMixer:
            return Assignment::AvatarMixerType;
        case NodeType::Agent:
            return Assignment::AgentType;
        case NodeType::EntityServer:
            return Assignment::EntityServerType;
        default:
            return Assignment::AllTypes;
    }
}

Assignment::Assignment() :
    _uuid(),
    _command(Assignment::RequestCommand),
    _type(Assignment::AllTypes),
    _pool(),
    _location(Assignment::LocalLocation),
    _payload(),
    _isStatic(false)
{
    
}

Assignment::Assignment(Assignment::Command command, Assignment::Type type, const QString& pool, Assignment::Location location) :
    _uuid(),
    _command(command),
    _type(type),
    _pool(pool),
    _location(location),
    _payload(),
    _isStatic(false),
    _walletUUID()
{
    if (_command == Assignment::CreateCommand) {
        // this is a newly created assignment, generate a random UUID
        _uuid = QUuid::createUuid();
    }
}

Assignment::Assignment(const QByteArray& packet) :
    _pool(),
    _location(GlobalLocation),
    _payload(),
    _walletUUID()
{
    PacketType::Value packetType = packetTypeForPacket(packet);
    
    if (packetType == PacketType::RequestAssignment) {
        _command = Assignment::RequestCommand;
    } else if (packetType == PacketType::CreateAssignment) {
        _command = Assignment::CreateCommand;
    }
    
    QDataStream packetStream(packet);
    packetStream.skipRawData(numBytesForPacketHeader(packet));
    
    packetStream >> *this;
}

#ifdef WIN32
#pragma warning(default:4351) 
#endif


Assignment::Assignment(const Assignment& otherAssignment) {
    
    _uuid = otherAssignment._uuid;
    
    _command = otherAssignment._command;
    _type = otherAssignment._type;
    _location = otherAssignment._location;
    _pool = otherAssignment._pool;
    _payload = otherAssignment._payload;
    _walletUUID = otherAssignment._walletUUID;
}

Assignment& Assignment::operator=(const Assignment& rhsAssignment) {
    Assignment temp(rhsAssignment);
    swap(temp);
    return *this;
}

void Assignment::swap(Assignment& otherAssignment) {
    using std::swap;
    
    swap(_uuid, otherAssignment._uuid);
    swap(_command, otherAssignment._command);
    swap(_type, otherAssignment._type);
    swap(_location, otherAssignment._location);
    swap(_pool, otherAssignment._pool);
    swap(_payload, otherAssignment._payload);
    swap(_walletUUID, otherAssignment._walletUUID);
}

const char* Assignment::getTypeName() const {
    switch (_type) {
        case Assignment::AudioMixerType:
            return "audio-mixer";
        case Assignment::AvatarMixerType:
            return "avatar-mixer";
        case Assignment::AgentType:
            return "agent";
        case Assignment::EntityServerType:
            return "entity-server";
        default:
            return "unknown";
    }
}

QDebug operator<<(QDebug debug, const Assignment &assignment) {
    debug.nospace() << "UUID: " << qPrintable(assignment.getUUID().toString()) <<
        ", Type: " << assignment.getType();
    
    if (!assignment.getPool().isEmpty()) {
        debug << ", Pool: " << assignment.getPool();
    }

    return debug.space();
}

QDataStream& operator<<(QDataStream &out, const Assignment& assignment) {
    out << (quint8) assignment._type << assignment._uuid << assignment._pool << assignment._payload;
    
    if (assignment._command == Assignment::RequestCommand) {
        out << assignment._walletUUID;
    }
    
    return out;
}

QDataStream& operator>>(QDataStream &in, Assignment& assignment) {
    quint8 packedType;
    in >> packedType;
    assignment._type = (Assignment::Type) packedType;
    
    in >> assignment._uuid >> assignment._pool >> assignment._payload;
    
    if (assignment._command == Assignment::RequestCommand) {
        in >> assignment._walletUUID;
    }
    
    return in;
}
