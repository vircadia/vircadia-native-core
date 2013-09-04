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
    _type(type)
{
    // copy the pool, if we got one
    if (pool) {
        int poolLength = strlen(pool);
        
        // create the char array and make it large enough for string and null termination
        _pool = new char[poolLength + sizeof(char)];
        memcpy(_pool, pool, poolLength + 1);
    }
}

Assignment::Assignment(const unsigned char* dataBuffer) {
    int numBytesRead = 0;
    
    memcpy(&_direction, dataBuffer, sizeof(Assignment::Direction));
    numBytesRead += sizeof(Assignment::Direction);
    
    memcpy(&_type, dataBuffer + numBytesRead, sizeof(Assignment::Type));
    numBytesRead += sizeof(Assignment::Type);
    
    int poolLength = strlen((const char*) dataBuffer + numBytesRead);
    _pool = new char[poolLength + sizeof(char)];
    memcpy(_pool, dataBuffer + numBytesRead, poolLength + sizeof(char));
}

int Assignment::packAssignmentToBuffer(unsigned char* buffer) {
    int numPackedBytes = 0;
    
    memcpy(buffer, &_direction, sizeof(_direction));
    numPackedBytes += sizeof(_direction);
    
    memcpy(buffer, &_type, sizeof(_type));
    numPackedBytes += sizeof(_type);
    
    strcpy((char *)buffer, _pool);
    numPackedBytes += strlen(_pool) + sizeof(char);
    
    return numPackedBytes;
}

QDebug operator<<(QDebug debug, const Assignment &assignment) {
    debug << "T:" << assignment.getType() << "P" << assignment.getPool();
    return debug.nospace();
}