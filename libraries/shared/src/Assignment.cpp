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
    _numberOfInstances(1),
    _payload(),
    _numPayloadBytes(0)
{
    
}

Assignment::Assignment(Assignment::Command command, Assignment::Type type, const QString& pool, Assignment::Location location) :
    _command(command),
    _type(type),
    _pool(pool),
    _location(location),
    _numberOfInstances(1),
    _payload(),
    _numPayloadBytes(0)
{
    if (_command == Assignment::CreateCommand) {
        // this is a newly created assignment, generate a random UUID
        _uuid = QUuid::createUuid();
    }
}

Assignment::Assignment(const Assignment& otherAssignment) {
    
    _uuid = otherAssignment._uuid;
    
    _command = otherAssignment._command;
    _type = otherAssignment._type;
    _location = otherAssignment._location;
    _pool = otherAssignment._pool;
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
    swap(_pool, otherAssignment._pool);
    swap(_numberOfInstances, otherAssignment._numberOfInstances);
    
    for (int i = 0; i < MAX_PAYLOAD_BYTES; i++) {
        swap(_payload[i], otherAssignment._payload[i]);
    }
    
    swap(_numPayloadBytes, otherAssignment._numPayloadBytes);
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
        _pool = QString((char*) dataBuffer + numBytesRead);
    } else {
        // skip past the null pool
        numBytesRead++;
    }

    if (numBytes > numBytesRead) {
        setPayload(dataBuffer + numBytesRead, numBytes - numBytesRead);
    }
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
    
    if (!_pool.isEmpty()) {
        // pack the pool for this assignment, it exists
        memcpy(buffer + numPackedBytes, _pool.toLocal8Bit().constData(), _pool.toLocal8Bit().size());
        numPackedBytes += _pool.toLocal8Bit().size();
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

void Assignment::run() {
    // run method ovveridden by subclasses
}

QDebug operator<<(QDebug debug, const Assignment &assignment) {
    debug.nospace() << "UUID: " << assignment.getUUID().toString().toStdString().c_str() <<
        ", Type: " << assignment.getType();
    return debug.nospace();
}
