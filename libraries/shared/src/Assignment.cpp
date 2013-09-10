//
//  Assignment.cpp
//  hifi
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "PacketHeaders.h"

#include "Assignment.h"

const char IPv4_ADDRESS_DESIGNATOR = 4;
const char IPv6_ADDRESS_DESIGNATOR = 6;

Assignment::Assignment(Assignment::Direction direction, Assignment::Type type, const char* pool) :
    _direction(direction),
    _type(type),
    _pool(NULL),
    _attachedPublicSocket(NULL),
    _attachedLocalSocket(NULL)
{
    // set the create time on this assignment
    gettimeofday(&_time, NULL);
    
    // copy the pool, if we got one
    if (pool) {
        int poolLength = strlen(pool);
        
        // create the char array and make it large enough for string and null termination
        _pool = new char[poolLength + sizeof(char)];
        strcpy(_pool, pool);
    }
}

Assignment::Assignment(const unsigned char* dataBuffer, int numBytes) :
    _pool(NULL),
    _attachedPublicSocket(NULL),
    _attachedLocalSocket(NULL)
{
    // set the create time on this assignment
    gettimeofday(&_time, NULL);
    
    int numBytesRead = 0;
    
    if (dataBuffer[0] == PACKET_TYPE_REQUEST_ASSIGNMENT) {
        _direction = Assignment::Request;
    } else if (dataBuffer[0] == PACKET_TYPE_CREATE_ASSIGNMENT) {
        _direction = Assignment::Create;
    } else if (dataBuffer[0] == PACKET_TYPE_DEPLOY_ASSIGNMENT) {
        _direction = Assignment::Deploy;
    }
    
    numBytesRead += numBytesForPacketHeader(dataBuffer);
    
    memcpy(&_type, dataBuffer + numBytesRead, sizeof(Assignment::Type));
    numBytesRead += sizeof(Assignment::Type);
    
    if (dataBuffer[numBytesRead] != 0) {
        int poolLength = strlen((const char*) dataBuffer + numBytesRead);
        _pool = new char[poolLength + sizeof(char)];
        strcpy(_pool, (char*) dataBuffer + numBytesRead);
        numBytesRead += poolLength + sizeof(char);
    } else {
        numBytesRead += sizeof(char);
    }
    
    if (numBytes > numBytesRead) {
        
        sockaddr* socketDestination = (_direction == Assignment::Create)
            ? _attachedLocalSocket
            : _attachedPublicSocket;
        
        if (dataBuffer[numBytesRead++] == IPv4_ADDRESS_DESIGNATOR) {
            // IPv4 address
            delete socketDestination;
            socketDestination = (sockaddr*) new sockaddr_in;
            unpackSocket(dataBuffer + numBytesRead, socketDestination);
        } else {
            // IPv6 address, or bad designator
            qDebug("Received a socket that cannot be unpacked!\n");
        }
    }
}

Assignment::~Assignment() {
    delete _attachedPublicSocket;
    delete _attachedLocalSocket;
    delete _pool;
}

void Assignment::setAttachedPublicSocket(const sockaddr* attachedPublicSocket) {
    
    if (_attachedPublicSocket) {
        // delete the old socket if it exists
        delete _attachedPublicSocket;
        _attachedPublicSocket = NULL;
    }
    
    copySocketToEmptySocketPointer(_attachedPublicSocket, attachedPublicSocket);
}

void Assignment::setAttachedLocalSocket(const sockaddr* attachedLocalSocket) {
    if (_attachedLocalSocket) {
        // delete the old socket if it exists
        delete _attachedLocalSocket;
        _attachedLocalSocket = NULL;
    }
    
    copySocketToEmptySocketPointer(_attachedLocalSocket, attachedLocalSocket);
}

int Assignment::packToBuffer(unsigned char* buffer) {
    int numPackedBytes = 0;
    
    memcpy(buffer + numPackedBytes, &_type, sizeof(_type));
    numPackedBytes += sizeof(_type);
    
    if (_pool) {
        int poolLength = strlen(_pool);
        strcpy((char*) buffer + numPackedBytes, _pool);
        
        numPackedBytes += poolLength + sizeof(char);
    } else {
        buffer[numPackedBytes] = '\0';
        numPackedBytes += sizeof(char);
    }
    
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
    debug << "T:" << assignment.getType() << "P:" << assignment.getPool();
    return debug.nospace();
}