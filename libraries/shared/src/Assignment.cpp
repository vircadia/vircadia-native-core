//
//  Assignment.cpp
//  hifi
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "PacketHeaders.h"
#include "SharedUtil.h"

#include "Assignment.h"

const char IPv4_ADDRESS_DESIGNATOR = 4;
const char IPv6_ADDRESS_DESIGNATOR = 6;

Assignment::Assignment(Assignment::Command command, Assignment::Type type, Assignment::Location location) :
    _command(command),
    _type(type),
    _location(location),
    _numberOfInstances(1),
    _payload(NULL),
    _numPayloadBytes(0)
{
    // set the create time on this assignment
    gettimeofday(&_time, NULL);
    
    if (_command == Assignment::CreateCommand) {
        // this is a newly created assignment, generate a random UUID
        _uuid = QUuid::createUuid();
    }
}

Assignment::Assignment(const unsigned char* dataBuffer, int numBytes) :
    _location(GlobalLocation),
    _numberOfInstances(1),
    _payload(NULL),
    _numPayloadBytes(0)
{
    // set the create time on this assignment
    gettimeofday(&_time, NULL);
    
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
    
    if (dataBuffer[0] != PACKET_TYPE_REQUEST_ASSIGNMENT) {
        // read the GUID for this assignment
        _uuid = QUuid::fromRfc4122(QByteArray((const char*) dataBuffer + numBytesRead, NUM_BYTES_RFC4122_UUID));
        numBytesRead += NUM_BYTES_RFC4122_UUID;
    }

    if (numBytes > numBytesRead) {
        _numPayloadBytes = numBytes - numBytesRead;
        _payload =  new uchar[_numPayloadBytes];
        memcpy(_payload, dataBuffer + numBytesRead, _numPayloadBytes);
    }
}

Assignment::~Assignment() {
    delete[] _payload;
    _numPayloadBytes = 0;
}

const int MAX_PAYLOAD_BYTES = 1024;

void Assignment::setPayload(const uchar* payload, int numBytes) {
    
    if (numBytes > MAX_PAYLOAD_BYTES) {
        qDebug("Set payload called with number of bytes greater than maximum (%d). Will only transfer %d bytes.\n",
               MAX_PAYLOAD_BYTES,
               MAX_PAYLOAD_BYTES);
        
        _numPayloadBytes = 1024;
    } else {
        _numPayloadBytes = numBytes;
    }
    
    delete[] _payload;
    _payload = new uchar[_numPayloadBytes];
    memcpy(_payload, payload, _numPayloadBytes);
}

QString Assignment::getUUIDStringWithoutCurlyBraces() const {
    return _uuid.toString().mid(1, _uuid.toString().length() - 2);
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
    debug << "T:" << assignment.getType();
    return debug.nospace();
}