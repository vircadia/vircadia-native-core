//
//  Assignment.cpp
//  hifi
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "PacketHeaders.h"

#include "Assignment.h"

Assignment::Assignment(Assignment::Direction direction, Assignment::Type type, const char* pool) :
    _direction(direction),
    _type(type),
    _pool(NULL),
    _domainSocket(NULL)
{
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
    _domainSocket(NULL)
{
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
        if (dataBuffer[numBytesRead++] == 4) {
            // IPv4 address
            sockaddr_in destinationSocket = {};
            memcpy(&destinationSocket, dataBuffer + numBytesRead, sizeof(sockaddr_in));
            destinationSocket.sin_family = AF_INET;
            setDomainSocket((sockaddr*) &destinationSocket);
        } else {
            // IPv6 address
            sockaddr_in6 destinationSocket = {};
            memcpy(&destinationSocket, dataBuffer + numBytesRead, sizeof(sockaddr_in6));
            setDomainSocket((sockaddr*) &destinationSocket);
        }
    }
}

Assignment::~Assignment() {
    delete _domainSocket;
    delete _pool;
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
    
    if (_domainSocket) {
        buffer[numPackedBytes++] = (_domainSocket->sa_family == AF_INET) ? 4 : 6;
        
        int numSocketBytes = (_domainSocket->sa_family == AF_INET) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
        
        memcpy(buffer + numPackedBytes, _domainSocket, numSocketBytes);
        numPackedBytes += numSocketBytes;
    }
    
    return numPackedBytes;
}

void Assignment::setDomainSocket(const sockaddr* domainSocket) {
    
    if (_domainSocket) {
        // delete the old _domainSocket if it exists
        delete _domainSocket;
        _domainSocket = NULL;
    }
    
    // create a new sockaddr or sockaddr_in depending on what type of address this is
    if (domainSocket->sa_family == AF_INET) {
        sockaddr_in* newSocket = new sockaddr_in;
        memcpy(newSocket, domainSocket, sizeof(sockaddr_in));
        _domainSocket = (sockaddr*) newSocket;
    } else {
        sockaddr_in6* newSocket = new sockaddr_in6;
        memcpy(newSocket, domainSocket, sizeof(sockaddr_in6));
        _domainSocket = (sockaddr*) newSocket;
    }
}

QDebug operator<<(QDebug debug, const Assignment &assignment) {
    debug << "T:" << assignment.getType() << "P:" << assignment.getPool();
    return debug.nospace();
}