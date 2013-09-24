//
//  Assignment.cpp
//  hifi
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "PacketHeaders.h"
#include "SharedUtil.h"

#include <QtCore/QDataStream>

#include "Assignment.h"

const char IPv4_ADDRESS_DESIGNATOR = 4;
const char IPv6_ADDRESS_DESIGNATOR = 6;

Assignment::Assignment() :
    _uuid(),
    _command(Assignment::RequestCommand),
    _type(Assignment::AllTypes),
    _location(Assignment::LocalLocation),
    _numberOfInstances(1),
    _payload(),
    _numPayloadBytes(0)
{
    
}

Assignment::Assignment(Assignment::Command command, Assignment::Type type, Assignment::Location location) :
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
}

Assignment::Assignment(const Assignment& otherAssignment) {
    
    _uuid = otherAssignment._uuid;
    
    _command = otherAssignment._command;
    _type = otherAssignment._type;
    _location = otherAssignment._location;
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

QDataStream& operator<<(QDataStream& out, const Assignment& assignment) {
    out << assignment._uuid;
    out << (uchar) assignment._command;
    out << (uchar) assignment._type;
    out << (uchar) assignment._location;
    out << assignment._numberOfInstances;
    out << assignment._numPayloadBytes;
    
    if (assignment._numPayloadBytes > 0) {
        out.writeBytes((char*) assignment._payload, assignment._numPayloadBytes);
    }
    
    return out;
}

QDataStream& operator>>(QDataStream& in, Assignment& assignment) {
    in >> assignment._uuid;
    in >> (uchar&) assignment._command;
    in >> (uchar&) assignment._type;
    in >> (uchar&) assignment._location;
    in >> assignment._numberOfInstances;
    in >> assignment._numPayloadBytes;
    
    if (assignment._numPayloadBytes > 0) {
        in.readRawData((char*) assignment._payload, assignment._numPayloadBytes);
    }
    
    return in;
}
