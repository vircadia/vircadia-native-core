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

const int NUM_BYTES_RFC4122_UUID = 16;

Assignment::Assignment(Assignment::Command command, Assignment::Type type, Assignment::Location location) :
    _command(command),
    _type(type),
    _location(location),
    _attachedPublicSocket(NULL),
    _attachedLocalSocket(NULL),
    _numberOfInstances(1)
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
    _attachedPublicSocket(NULL),
    _attachedLocalSocket(NULL),
    _numberOfInstances(1)
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
    
    if (dataBuffer[0] != PACKET_TYPE_REQUEST_ASSIGNMENT) {
        // read the GUID for this assignment
        _uuid = QUuid::fromRfc4122(QByteArray((const char*) dataBuffer + numBytesRead, NUM_BYTES_RFC4122_UUID));
        numBytesRead += NUM_BYTES_RFC4122_UUID;
    }
    
    memcpy(&_type, dataBuffer + numBytesRead, sizeof(Assignment::Type));
    numBytesRead += sizeof(Assignment::Type);
    
    if (numBytes > numBytesRead) {
        
        sockaddr* newSocket = NULL;
        
        if (dataBuffer[numBytesRead++] == IPv4_ADDRESS_DESIGNATOR) {
            // IPv4 address
            newSocket = (sockaddr*) new sockaddr_in;
            unpackSocket(dataBuffer + numBytesRead, newSocket);
        } else {
            // IPv6 address, or bad designator
            qDebug("Received a socket that cannot be unpacked!\n");
        }
        
        if (_command == Assignment::CreateCommand) {
            delete _attachedLocalSocket;
            _attachedLocalSocket = newSocket;
        } else {
            delete _attachedPublicSocket;
            _attachedPublicSocket = newSocket;
        }
    }
}

Assignment::~Assignment() {
    delete _attachedPublicSocket;
    delete _attachedLocalSocket;
}

QString Assignment::getUUIDStringWithoutCurlyBraces() const {
    return _uuid.toString().mid(1, _uuid.toString().length() - 2);
}

void Assignment::setAttachedPublicSocket(const sockaddr* attachedPublicSocket) {
    if (_attachedPublicSocket) {
        // delete the old socket if it exists
        delete _attachedPublicSocket;
        _attachedPublicSocket = NULL;
    }
    
    if (attachedPublicSocket) {
        copySocketToEmptySocketPointer(&_attachedPublicSocket, attachedPublicSocket);
    }
}

void Assignment::setAttachedLocalSocket(const sockaddr* attachedLocalSocket) {
    if (_attachedLocalSocket) {
        // delete the old socket if it exists
        delete _attachedLocalSocket;
        _attachedLocalSocket = NULL;
    }
    
    if (attachedLocalSocket) {
        copySocketToEmptySocketPointer(&_attachedLocalSocket, attachedLocalSocket);
    }
}

int Assignment::packToBuffer(unsigned char* buffer) {
    int numPackedBytes = 0;
    
    // pack the UUID for this assignment, if this is an assignment create or deploy
    if (_command != Assignment::RequestCommand) {
        memcpy(buffer, _uuid.toRfc4122().constData(), NUM_BYTES_RFC4122_UUID);
        numPackedBytes += NUM_BYTES_RFC4122_UUID;
    }
    
    memcpy(buffer + numPackedBytes, &_type, sizeof(_type));
    numPackedBytes += sizeof(_type);
    
    if (_attachedPublicSocket || _attachedLocalSocket) {
        sockaddr* socketToPack = (_attachedPublicSocket) ? _attachedPublicSocket : _attachedLocalSocket;
        
        // we have a socket to pack, add the designator
        buffer[numPackedBytes++] = (socketToPack->sa_family == AF_INET)
            ? IPv4_ADDRESS_DESIGNATOR : IPv6_ADDRESS_DESIGNATOR;
        
        numPackedBytes += packSocket(buffer + numPackedBytes, socketToPack);
    }

    return numPackedBytes;
}

QDebug operator<<(QDebug debug, const Assignment &assignment) {
    debug << "T:" << assignment.getType();
    return debug.nospace();
}