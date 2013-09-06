//
//  Assignment.h
//  hifi
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__Assignment__
#define __hifi__Assignment__

#include "NodeList.h"

class Assignment {
public:
    
    enum Type {
        AudioMixer,
        AvatarMixer,
        All
    };
    
    enum Direction {
        Create,
        Deploy,
        Request
    };
    
    Assignment(Assignment::Direction direction, Assignment::Type type, const char* pool = NULL);
    Assignment(const unsigned char* dataBuffer, int numBytes);
    Assignment(const Assignment& assignment);
    ~Assignment();
    
    Assignment::Direction getDirection() const { return _direction; }
    Assignment::Type getType() const { return _type; }
    const char* getPool() const { return _pool; }
    
    const sockaddr* getDomainSocket() { return _domainSocket; }
    void setDomainSocket(const sockaddr* domainSocket);
    
    int packToBuffer(unsigned char* buffer);
    
private:
    Assignment::Direction _direction;
    Assignment::Type _type;
    char* _pool;
    sockaddr* _domainSocket;
};

QDebug operator<<(QDebug debug, const Assignment &assignment);

#endif /* defined(__hifi__Assignment__) */
