//
//  Assignment.cpp
//  hifi
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "Assignment.h"

Assignment::Assignment(Assignment::Direction direction, Assignment::Type type, const char* pool) :
    _direction(direction),
    _type(type),
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
    _domainSocket(NULL)
{
    int numBytesRead = 0;
    
    memcpy(&_direction, dataBuffer, sizeof(Assignment::Direction));
    numBytesRead += sizeof(Assignment::Direction);
    
    memcpy(&_type, dataBuffer + numBytesRead, sizeof(Assignment::Type));
    numBytesRead += sizeof(Assignment::Type);
    
    int poolLength = strlen((const char*) dataBuffer + numBytesRead);
    _pool = new char[poolLength + sizeof(char)];
    strcpy(_pool, (char*) dataBuffer + numBytesRead);
    numBytesRead += poolLength + sizeof(char);
    
    if (numBytes > numBytesRead) {
        memcpy(_domainSocket, dataBuffer + numBytesRead, sizeof(sockaddr));
    }
}

int Assignment::packToBuffer(unsigned char* buffer) {
    int numPackedBytes = 0;
    
    memcpy(buffer, &_direction, sizeof(_direction));
    numPackedBytes += sizeof(_direction);
    
    memcpy(buffer + numPackedBytes, &_type, sizeof(_type));
    numPackedBytes += sizeof(_type);
    
    strcpy((char*) buffer + numPackedBytes, _pool);
    numPackedBytes += strlen(_pool) + sizeof(char);
    
    if (_domainSocket) {
        memcpy(buffer + numPackedBytes, _domainSocket, sizeof(sockaddr));
        numPackedBytes += sizeof(sockaddr);
    }
    
    return numPackedBytes;
}

void Assignment::setDomainSocket(const sockaddr* domainSocket) {
    
    if (_domainSocket) {
        // delete the old _domainSocket if it exists
        delete _domainSocket;
        _domainSocket = NULL;
    }
    
    if (!_domainSocket) {
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
}

QDebug operator<<(QDebug debug, const Assignment &assignment) {
    debug << "T:" << assignment.getType() << "P:" << assignment.getPool();
    return debug.nospace();
}