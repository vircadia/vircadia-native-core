//
//  Assignment.h
//  hifi
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__Assignment__
#define __hifi__Assignment__

#include <sys/time.h>

#include <QtCore/QUuid>

#include "NodeList.h"

const int MAX_PAYLOAD_BYTES = 1024;
const int MAX_ASSIGNMENT_POOL_BYTES = 64 + sizeof('\0');

/// Holds information used for request, creation, and deployment of assignments
class Assignment : public NodeData {
    Q_OBJECT
public:
    
    enum Type {
        AudioMixerType,
        AvatarMixerType,
        AgentType,
        VoxelServerType,
        ParticleServerType,
        AllTypes
    };
    
    enum Command {
        CreateCommand,
        DeployCommand,
        RequestCommand
    };
    
    enum Location {
        GlobalLocation,
        LocalLocation
    };
    
    static Assignment::Type typeForNodeType(NODE_TYPE nodeType);
    
    Assignment();
    Assignment(Assignment::Command command,
               Assignment::Type type,
               const char* pool = NULL,
               Assignment::Location location = Assignment::LocalLocation);
    Assignment(const Assignment& otherAssignment);
    Assignment& operator=(const Assignment &rhsAssignment);
    
    void swap(Assignment& otherAssignment);
    
    /// Constructs an Assignment from the data in the buffer
    /// \param dataBuffer the source buffer to un-pack the assignment from
    /// \param numBytes the number of bytes left to read in the source buffer
    Assignment(const unsigned char* dataBuffer, int numBytes);
    
    void setUUID(const QUuid& uuid) { _uuid = uuid; }
    const QUuid& getUUID() const { return _uuid; }
    void resetUUID() { _uuid = QUuid::createUuid(); }
    
    Assignment::Command getCommand() const { return _command; }
    Assignment::Type getType() const { return _type; }
    Assignment::Location getLocation() const { return _location; }
    
    uchar* getPayload() { return _payload; }
    int getNumPayloadBytes() const { return _numPayloadBytes; }
    void setPayload(const uchar *payload, int numBytes);
    
    void setPool(const char* pool);
    const char* getPool() const { return _pool; }
    bool hasPool() const { return (bool) strlen(_pool); }
    
    int getNumberOfInstances() const { return _numberOfInstances; }
    void setNumberOfInstances(int numberOfInstances) { _numberOfInstances = numberOfInstances; }
    void decrementNumberOfInstances() { --_numberOfInstances; }
    
    const char* getTypeName() const;

    /// Packs the assignment to the passed buffer
    /// \param buffer the buffer in which to pack the assignment
    /// \return number of bytes packed into buffer
    int packToBuffer(unsigned char* buffer);
    
    // implement parseData to return 0 so we can be a subclass of NodeData
    int parseData(unsigned char* sourceBuffer, int numBytes) { return 0; }
    
    friend QDebug operator<<(QDebug debug, const Assignment& assignment);
    friend QDataStream& operator<<(QDataStream &out, const Assignment& assignment);
    friend QDataStream& operator>>(QDataStream &in, Assignment& assignment);
    
protected:
    QUuid _uuid; /// the 16 byte UUID for this assignment
    Assignment::Command _command; /// the command for this assignment (Create, Deploy, Request)
    Assignment::Type _type; /// the type of the assignment, defines what the assignee will do
    char _pool[MAX_ASSIGNMENT_POOL_BYTES]; /// the destination pool for this assignment
    Assignment::Location _location; /// the location of the assignment, allows a domain to preferentially use local ACs
    int _numberOfInstances; /// the number of instances of this assignment
    uchar _payload[MAX_PAYLOAD_BYTES]; /// an optional payload attached to this assignment, a maximum for 1024 bytes will be packed
    int _numPayloadBytes; /// number of bytes in the payload, up to a maximum of 1024
};

#endif /* defined(__hifi__Assignment__) */
