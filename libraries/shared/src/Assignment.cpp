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
    _domainSocket(NULL)
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
    _domainSocket(NULL)
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
        if (dataBuffer[numBytesRead++] == IPv4_ADDRESS_DESIGNATOR) {
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

void Assignment::setDomainSocket(const sockaddr* domainSocket) {
    
    if (_domainSocket) {
        // delete the old _domainSocket if it exists
        delete _domainSocket;
        _domainSocket = NULL;
    }
    
    // create a new sockaddr or sockaddr_in depending on what type of address this is
    if (domainSocket->sa_family == AF_INET) {
        _domainSocket = (sockaddr*) new sockaddr_in;
        memcpy(_domainSocket, domainSocket, sizeof(sockaddr_in));
    } else {
        _domainSocket = (sockaddr*) new sockaddr_in6;
        memcpy(_domainSocket, domainSocket, sizeof(sockaddr_in6));
    }
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
        buffer[numPackedBytes++] = (_domainSocket->sa_family == AF_INET) ? IPv4_ADDRESS_DESIGNATOR : IPv6_ADDRESS_DESIGNATOR;
        
        int numSocketBytes = (_domainSocket->sa_family == AF_INET) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
        
        memcpy(buffer + numPackedBytes, _domainSocket, numSocketBytes);
        numPackedBytes += numSocketBytes;
    }
    
    return numPackedBytes;
}

QString Assignment::toString() const {
    return QString("T:%1 P:%2").arg(_type).arg(_pool);
}

QDebug operator<<(QDebug debug, const Assignment &assignment) {
    debug << assignment.toString().toStdString().c_str();
    return debug.nospace();
}