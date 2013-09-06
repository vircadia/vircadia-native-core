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

/// Holds information used for request, creation, and deployment of assignments
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
    
    /// Constructs an Assignment from the data in the buffer
    /// \param dataBuffer the source buffer to un-pack the assignment from
    /// \param numBytes the number of bytes left to read in the source buffer
    Assignment(const unsigned char* dataBuffer, int numBytes);
    
    ~Assignment();
    
    Assignment::Direction getDirection() const { return _direction; }
    Assignment::Type getType() const { return _type; }
    const char* getPool() const { return _pool; }
    const timeval& getTime() const { return _time; }
    
    const sockaddr* getDomainSocket() { return _domainSocket; }
    void setDomainSocket(const sockaddr* domainSocket);
    
    /// Packs the assignment to the passed buffer
    /// \param buffer the buffer in which to pack the assignment
    /// \return number of bytes packed into buffer
    int packToBuffer(unsigned char* buffer);
    
private:
    Assignment::Direction _direction; /// the direction of the assignment (Create, Deploy, Request)
    Assignment::Type _type; /// the type of the assignment, defines what the assignee will do
    char* _pool; /// the pool this assignment is for/from
    sockaddr* _domainSocket; /// pointer to socket for domain server that created assignment
    timeval _time; /// time the assignment was created (set in constructor)
};

QDebug operator<<(QDebug debug, const Assignment &assignment);

#endif /* defined(__hifi__Assignment__) */
