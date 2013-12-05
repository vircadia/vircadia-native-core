//
//  Assignment.cpp
//  hifi
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "PacketHeaders.h"
#include "SharedUtil.h"
#include "UUID.h"

#include <QtCore/QDataStream>

#include "Assignment.h"

const char IPv4_ADDRESS_DESIGNATOR = 4;
const char IPv6_ADDRESS_DESIGNATOR = 6;

Assignment::Type Assignment::typeForNodeType(NODE_TYPE nodeType) {
    switch (nodeType) {
        case NODE_TYPE_AUDIO_MIXER:
            return Assignment::AudioMixerType;
        case NODE_TYPE_AVATAR_MIXER:
            return Assignment::AvatarMixerType;
        case NODE_TYPE_AGENT:
            return Assignment::AgentType;
        case NODE_TYPE_VOXEL_SERVER:
            return Assignment::VoxelServerType;
        case NODE_TYPE_PARTICLE_SERVER:
            return Assignment::ParticleServerType;
        default:
            return Assignment::AllTypes;
    }
}

Assignment::Assignment() :
    _uuid(),
    _command(Assignment::RequestCommand),
    _type(Assignment::AllTypes),
    _location(Assignment::LocalLocation),
    _numberOfInstances(1),
    _payload(),
    _numPayloadBytes(0)
{
    setPool(NULL);
}

Assignment::Assignment(Assignment::Command command, Assignment::Type type, const char* pool, Assignment::Location location) :
    _command(command),
    _type(type),
    _location(location),
    _numberOfInstances(1),
    _payload(),
    _numPayloadBytes(0)
{
    if (_command == Assignment::CreateCommand) {
        // this is a newly created assignment, generate a random UUID
        _uuid = QUuid::createUuid();
    }
    
    setPool(pool);
}

Assignment::Assignment(const unsigned char* dataBuffer, int numBytes) :
    _location(GlobalLocation),
    _numberOfInstances(1),
    _payload(),
    _numPayloadBytes(0)
{
    int numBytesRead = 0;
    
    if (dataBuffer[0] == PACKET_TYPE_REQUEST_ASSIGNMENT) {
        _command = Assignment::RequestCommand;
    } else if (dataBuffer[0] == PACKET_TYPE_CREATE_ASSIGNMENT) {
        _command = Assignment::CreateCommand;
    } else if (dataBuffer[0] == PACKET_TYPE_DEPLOY_ASSIGNMENT) {
        _command = Assignment::DeployCommand;
    }
    
    numBytesRead += numBytesForPacketHeader(dataBuffer);
    
    memcpy(&_type, dataBuffer + numBytesRead, sizeof(Assignment::Type));
    numBytesRead += sizeof(Assignment::Type);
    
    if (_command != Assignment::RequestCommand) {
        // read the GUID for this assignment
        _uuid = QUuid::fromRfc4122(QByteArray((const char*) dataBuffer + numBytesRead, NUM_BYTES_RFC4122_UUID));
        numBytesRead += NUM_BYTES_RFC4122_UUID;
    }
    
    if (dataBuffer[numBytesRead] != '\0') {
        // read the pool from the data buffer
        setPool((const char*) dataBuffer + numBytesRead);
        numBytesRead += strlen(_pool) + sizeof('\0');
    } else {
        // skip past the null pool and null out our pool
        setPool(NULL);
        numBytesRead++;
    }
    
    if (numBytes > numBytesRead) {
        setPayload(dataBuffer + numBytesRead, numBytes - numBytesRead);
    }
}

Assignment::Assignment(const Assignment& otherAssignment) {
    
    _uuid = otherAssignment._uuid;
    
    _command = otherAssignment._command;
    _type = otherAssignment._type;
    _location = otherAssignment._location;
    setPool(otherAssignment._pool);
    _numberOfInstances = otherAssignment._numberOfInstances;
    
    setPayload(otherAssignment._payload, otherAssignment._numPayloadBytes);
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
    
    for (int i = 0; i < sizeof(_pool); i++) {
        swap(_pool[i], otherAssignment._pool[i]);
    }
    
    swap(_numberOfInstances, otherAssignment._numberOfInstances);
    
    for (int i = 0; i < MAX_PAYLOAD_BYTES; i++) {
        swap(_payload[i], otherAssignment._payload[i]);
    }
    
    swap(_numPayloadBytes, otherAssignment._numPayloadBytes);
}

void Assignment::setPayload(const uchar* payload, int numBytes) {
    
    if (numBytes > MAX_PAYLOAD_BYTES) {
        qDebug("Set payload called with number of bytes greater than maximum (%d). Will only transfer %d bytes.\n",
               MAX_PAYLOAD_BYTES,
               MAX_PAYLOAD_BYTES);
        
        _numPayloadBytes = 1024;
    } else {
        _numPayloadBytes = numBytes;
    }
    
    memset(_payload, 0, MAX_PAYLOAD_BYTES);
    memcpy(_payload, payload, _numPayloadBytes);
}

void Assignment::setPool(const char* pool) {
    memset(_pool, '\0', sizeof(_pool));
    
    if (pool) {
        strcpy(_pool, pool);
    }
}

const char* Assignment::getTypeName() const {
    switch (_type) {
        case Assignment::AudioMixerType:
            return "audio-mixer";
        case Assignment::AvatarMixerType:
            return "avatar-mixer";
        case Assignment::AgentType:
            return "agent";
        case Assignment::VoxelServerType:
            return "voxel-server";
        case Assignment::ParticleServerType:
            return "particle-server";
        default:
            return "unknown";
    }
}

int Assignment::packToBuffer(unsigned char* buffer) {
    int numPackedBytes = 0;
    
    memcpy(buffer + numPackedBytes, &_type, sizeof(_type));
    numPackedBytes += sizeof(_type);
    
    // pack the UUID for this assignment, if this is an assignment create or deploy
    if (_command != Assignment::RequestCommand) {
        memcpy(buffer + numPackedBytes, _uuid.toRfc4122().constData(), NUM_BYTES_RFC4122_UUID);
        numPackedBytes += NUM_BYTES_RFC4122_UUID;
    }
    
    if (hasPool()) {
        // pack the pool for this assignment, it exists
        int numBytesNullTerminatedPool = strlen(_pool) + sizeof('\0');
        memcpy(buffer + numPackedBytes, _pool, numBytesNullTerminatedPool);
        numPackedBytes += numBytesNullTerminatedPool;
    } else {
        // otherwise pack the null character
        buffer[numPackedBytes++] = '\0';
    }
    
    if (_numPayloadBytes) {
        memcpy(buffer + numPackedBytes, _payload, _numPayloadBytes);
        numPackedBytes += _numPayloadBytes;
    }
    
    return numPackedBytes;
}

QDebug operator<<(QDebug debug, const Assignment &assignment) {
    debug.nospace() << "UUID: " << assignment.getUUID().toString().toStdString().c_str() <<
        ", Type: " << assignment.getType();
    return debug.nospace();
}
